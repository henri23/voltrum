#include "vulkan_object_shader.hpp"
#include "renderer/vulkan/vulkan_buffer.hpp"
#include "renderer/vulkan/vulkan_pipeline.hpp"
#include "renderer/vulkan/vulkan_types.hpp"

#include "core/logger.hpp"
#include "renderer/vulkan/vulkan_shader_utils.hpp"

#include "math/math_types.hpp"

#define BUILTIN_SHADER_NAME_OBJECT "Builtin.ObjectShader"

b8 vulkan_object_shader_create(Vulkan_Context* context,
    Vulkan_Object_Shader* out_shader) {

    char stage_type_strs[OBJECT_SHADER_STAGE_COUNT][5] = {"vert", "frag"};

    VkShaderStageFlagBits state_types[OBJECT_SHADER_STAGE_COUNT] = {
        VK_SHADER_STAGE_VERTEX_BIT,
        VK_SHADER_STAGE_FRAGMENT_BIT};

    for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
        if (!create_shader_module(context,
                BUILTIN_SHADER_NAME_OBJECT,
                stage_type_strs[i],
                state_types[i],
                i,
                out_shader->stages)) {

            CORE_ERROR("Failed to create %s shader module for '%s'",
                stage_type_strs[i],
                BUILTIN_SHADER_NAME_OBJECT);

            return false;
        }
    }

    // Global descriptors
    VkDescriptorSetLayoutBinding global_ubo_layout_binding;
    global_ubo_layout_binding.binding = 0;
    global_ubo_layout_binding.descriptorCount = 1;
    global_ubo_layout_binding.descriptorType =
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    global_ubo_layout_binding.pImmutableSamplers = 0;
    // Since we currently have descriptors only for the vertex shader, we use
    // only the vertex bit
    global_ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo global_layout_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    global_layout_info.bindingCount = 1;
    global_layout_info.pBindings = &global_ubo_layout_binding;

    VK_CHECK(vkCreateDescriptorSetLayout(context->device.logical_device,
        &global_layout_info,
        context->allocator,
        &out_shader->global_descriptor_set_layout));

    VkDescriptorPoolSize global_pool_size;
    global_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    global_pool_size.descriptorCount = context->swapchain.image_count;

    VkDescriptorPoolCreateInfo global_pool_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    global_pool_info.poolSizeCount = 1;
    global_pool_info.pPoolSizes = &global_pool_size;
    global_pool_info.maxSets = context->swapchain.image_count;

    VK_CHECK(vkCreateDescriptorPool(context->device.logical_device,
        &global_pool_info,
        context->allocator,
        &out_shader->global_descriptor_pool));

    // Pipeline creation
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = (f32)context->main_target.height;
    viewport.width = (f32)context->main_target.width;
    viewport.height = -(f32)context->main_target.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = context->main_target.width;
    scissor.extent.height = context->main_target.height;

    u32 offset = 0;
    constexpr s32 attribute_count = 1;

    VkVertexInputAttributeDescription attribute_descriptions[attribute_count];

    // Position
    VkFormat formats[attribute_count] = {VK_FORMAT_R32G32B32_SFLOAT};
    u64 sizes[attribute_count] = {sizeof(vec3)};

    for (u32 i = 0; i < attribute_count; ++i) {
        attribute_descriptions[i].binding = 0;
        attribute_descriptions[i].location = i;
        attribute_descriptions[i].format = formats[i];
        attribute_descriptions[i].offset = offset;
        offset += sizes[i];
    }

    constexpr s32 descriptor_set_layout_count = 1;
    VkDescriptorSetLayout layouts[1] = {
        out_shader->global_descriptor_set_layout};

    VkPipelineShaderStageCreateInfo
        stage_create_infos[OBJECT_SHADER_STAGE_COUNT];

    memory_zero(stage_create_infos, sizeof(stage_create_infos));
    for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
        stage_create_infos[i].sType =
            out_shader->stages[i].shader_stage_create_info.sType;

        stage_create_infos[i] = out_shader->stages[i].shader_stage_create_info;
    }

    if (!vulkan_graphics_pipeline_create(context,
            &context->main_renderpass,
            attribute_count,
            attribute_descriptions,
            descriptor_set_layout_count,
            layouts,
            OBJECT_SHADER_STAGE_COUNT,
            stage_create_infos,
            viewport,
            scissor,
            false,
            &out_shader->pipeline)) {

        CORE_ERROR("Failed to load graphics pipeline for object shader");
        return false;
    }

    if (!vulkan_buffer_create(context,
            sizeof(Global_Uniform_Object) * context->swapchain.image_count,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            true,
            &out_shader->global_uniform_buffer)) {

        CORE_ERROR("Vulkan buffer creation failed for object shader.");
        return false;
    }

    Auto_Array<VkDescriptorSetLayout> global_layouts;

    for (u32 i = 0; i < context->swapchain.image_count; ++i)
        global_layouts.push_back(out_shader->global_descriptor_set_layout);

    VkDescriptorSetAllocateInfo alloc_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc_info.descriptorPool = out_shader->global_descriptor_pool;
    alloc_info.descriptorSetCount = context->swapchain.image_count;
    alloc_info.pSetLayouts = global_layouts.data;

    // Reserve memory in the global_descriptor_sets array equal to the number
    // of swapchain images
    out_shader->global_descriptor_sets =
        static_cast<VkDescriptorSet*>(memory_allocate(
            sizeof(VkDescriptorSet) * context->swapchain.image_count,
            Memory_Tag::RENDERER));

    VK_CHECK(vkAllocateDescriptorSets(context->device.logical_device,
        &alloc_info,
        out_shader->global_descriptor_sets));

    return true;
}

void vulkan_object_shader_destroy(Vulkan_Context* context,
    Vulkan_Object_Shader* shader) {

    memory_deallocate(shader->global_descriptor_sets,
        sizeof(VkDescriptorSet) * context->swapchain.image_count,
        Memory_Tag::RENDERER);

    VkDevice logical_device = context->device.logical_device;

    vulkan_buffer_destroy(context, &shader->global_uniform_buffer);

    vulkan_graphics_pipeline_destroy(context, &shader->pipeline);

    vkDestroyDescriptorPool(logical_device,
        shader->global_descriptor_pool,
        context->allocator);

    vkDestroyDescriptorSetLayout(logical_device,
        shader->global_descriptor_set_layout,
        context->allocator);

    for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i) {
        vkDestroyShaderModule(context->device.logical_device,
            shader->stages[i].handle,
            context->allocator);
    }
}

void vulkan_object_shader_use(Vulkan_Context* context,
    Vulkan_Object_Shader* shader) {
    u32 image_index = context->image_index;
    vulkan_graphics_pipeline_bind(&context->main_command_buffers[image_index],
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        &shader->pipeline);
}

// Not all GPUs are capable of performing an update operation after a bind
// operation for the descriptor sets for instead I need to implement the updated
// pattern, because nertheless I need to just bind once
void vulkan_object_shader_update_global_state(Vulkan_Context* context,
    Vulkan_Object_Shader* shader) {

    u32 image_index = context->image_index;
    VkCommandBuffer command_buffer =
        context->main_command_buffers[image_index].handle;
    VkDescriptorSet global_descriptor =
        shader->global_descriptor_sets[image_index];

    u32 range = sizeof(Global_Uniform_Object);
    u64 offset = sizeof(Global_Uniform_Object) * image_index;

    // Update uniform buffer data every frame with the correct offset for this
    // image
    vulkan_buffer_load_data(context,
        &shader->global_uniform_buffer,
        offset,
        range,
        0,
        &shader->global_ubo);

    // Only update descriptor sets once (to avoid GPU compatibility issues with
    // bind/update order)
    VkDescriptorBufferInfo bufferInfo;
    bufferInfo.buffer = shader->global_uniform_buffer.handle;
    bufferInfo.offset = offset;
    bufferInfo.range = range;

    // Update descriptor sets
    VkWriteDescriptorSet descriptor_write = {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptor_write.dstSet = shader->global_descriptor_sets[image_index];
    descriptor_write.dstBinding = 0;
    descriptor_write.dstArrayElement = 0;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_write.descriptorCount = 1;
    descriptor_write.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(context->device.logical_device,
        1,
        &descriptor_write,
        0,
        0);

    vkCmdBindDescriptorSets(command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        shader->pipeline.pipeline_layout,
        0,
        1,
        &global_descriptor,
        0,
        0);
}
