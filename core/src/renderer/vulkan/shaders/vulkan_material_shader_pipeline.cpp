#include "vulkan_material_shader_pipeline.hpp"
#include "defines.hpp"

#include "renderer/vulkan/vulkan_buffer.hpp"
#include "renderer/vulkan/vulkan_pipeline.hpp"
#include "renderer/vulkan/vulkan_types.hpp"

#include "core/logger.hpp"
#include "core/thread_context.hpp"
#include "memory/arena.hpp"
#include "renderer/vulkan/vulkan_shader_utils.hpp"

#include "math/math_types.hpp"
#include "systems/texture_system.hpp"

#define BUILTIN_SHADER_NAME_MATERIAL "Builtin.MaterialShader"

b8 vulkan_material_shader_pipeline_create(Vulkan_Context *context,
    Vulkan_Material_Shader_Pipeline *out_shader) {

    char stage_type_strs[VULKAN_MATERIAL_SHADER_STAGE_COUNT][5] = {"vert",
        "frag"};

    VkShaderStageFlagBits state_types[VULKAN_MATERIAL_SHADER_STAGE_COUNT] = {
        VK_SHADER_STAGE_VERTEX_BIT,
        VK_SHADER_STAGE_FRAGMENT_BIT};

    for (u32 i = 0; i < VULKAN_MATERIAL_SHADER_STAGE_COUNT; ++i) {
        if (!create_shader_module(context,
                BUILTIN_SHADER_NAME_MATERIAL,
                stage_type_strs[i],
                state_types[i],
                i,
                out_shader->stages)) {

            CORE_ERROR("Failed to create %s shader module for '%s'",
                stage_type_strs[i],
                BUILTIN_SHADER_NAME_MATERIAL);

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

    // Sampler_uses
    out_shader->sampler_uses[0] = Texture_Type::MAP_DIFFUSE;

    // Local descriptors
    VkDescriptorType descriptor_types[VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT] =
        {
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // Binding 0 - Uniform buffer
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER // Binding 1 - Diffuse
                                                      // sampler
        };

    VkDescriptorSetLayoutBinding
        bindings[VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT];

    memory_zero(&bindings,
        sizeof(VkDescriptorSetLayoutBinding) *
            VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT);

    for (u32 i = 0; i < VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT; ++i) {
        bindings[i].binding = i;
        bindings[i].descriptorCount = 1;
        bindings[i].descriptorType = descriptor_types[i];
        bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    VkDescriptorSetLayoutCreateInfo layout_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layout_info.bindingCount = VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT;
    layout_info.pBindings = bindings;
    VK_CHECK(vkCreateDescriptorSetLayout(context->device.logical_device,
        &layout_info,
        context->allocator,
        &out_shader->object_descriptor_set_layout));

    // Local object descriptor pool to be used for object specific diffuse color
    VkDescriptorPoolSize object_pool_sizes[2];
    // The first section will be used for uniform buffers
    object_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    object_pool_sizes[0].descriptorCount = VULKAN_MAX_MATERIAL_COUNT;

    object_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    object_pool_sizes[1].descriptorCount =
        VULKAN_MATERIAL_SHADER_SAMPLER_COUNT * VULKAN_MAX_MATERIAL_COUNT;

    VkDescriptorPoolCreateInfo object_pool_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    object_pool_info.poolSizeCount = 2;
    object_pool_info.pPoolSizes = object_pool_sizes;
    object_pool_info.maxSets = VULKAN_MAX_MATERIAL_COUNT;
    object_pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    // Create object descriptor pool
    VK_CHECK(vkCreateDescriptorPool(context->device.logical_device,
        &object_pool_info,
        context->allocator,
        &out_shader->object_descriptor_pool));

    // Pipeline creation
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = (f32)context->viewport.framebuffer_height;
    viewport.width = (f32)context->viewport.framebuffer_width;
    viewport.height = -(f32)context->viewport.framebuffer_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = context->viewport.framebuffer_width;
    scissor.extent.height = context->viewport.framebuffer_height;

    // Attributes
    u32 offset = 0;
    constexpr u32 attribute_count = 2;

    VkVertexInputAttributeDescription attribute_descriptions[attribute_count];

    // Position, Texture coordinates
    VkFormat formats[attribute_count] = {VK_FORMAT_R32G32B32_SFLOAT,
        VK_FORMAT_R32G32_SFLOAT};

    u64 sizes[attribute_count] = {sizeof(vec3), sizeof(vec2)};

    for (u32 i = 0; i < attribute_count; ++i) {
        attribute_descriptions[i].binding = 0;
        attribute_descriptions[i].location = i;
        attribute_descriptions[i].format = formats[i];
        attribute_descriptions[i].offset = offset;
        offset += sizes[i];
    }

    constexpr s32 descriptor_set_layout_count = 2;
    VkDescriptorSetLayout layouts[descriptor_set_layout_count] = {
        out_shader->global_descriptor_set_layout,
        out_shader->object_descriptor_set_layout};

    VkPipelineShaderStageCreateInfo
        stage_create_infos[VULKAN_MATERIAL_SHADER_STAGE_COUNT];

    memory_zero(stage_create_infos, sizeof(stage_create_infos));
    for (u32 i = 0; i < VULKAN_MATERIAL_SHADER_STAGE_COUNT; ++i) {
        stage_create_infos[i].sType =
            out_shader->stages[i].shader_stage_create_info.sType;

        stage_create_infos[i] = out_shader->stages[i].shader_stage_create_info;
    }

    if (!vulkan_graphics_pipeline_create(context,
            &context->viewport_renderpass,
            sizeof(vertex_3d),
            attribute_count,
            attribute_descriptions,
            descriptor_set_layout_count,
            layouts,
            VULKAN_MATERIAL_SHADER_STAGE_COUNT,
            stage_create_infos,
            viewport,
            scissor,
            false,
            true,
            &out_shader->pipeline)) {

        CORE_ERROR("Failed to load graphics pipeline for object shader");
        return false;
    }

    // NOTE: Some GPUs do not have the feature to provide a vulkan buffer that
    // is both DEVICE_LOCAL and HOST_VISIBLE. While we would want to prioritize
    // the device locality for performance, just leaving HOST_VISIBLE could be a
    // viable workaround for such GPUs
    u32 device_local_bits = context->device.supports_device_local_host_visible
                                ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                                : 0;

    if (!vulkan_buffer_create(context,
            sizeof(Vulkan_Material_Shader_Global_Ubo) *
                context->swapchain.image_count,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            device_local_bits | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            true,
            &out_shader->global_uniform_buffer)) {

        CORE_ERROR("Vulkan buffer creation failed for object shader.");
        return false;
    }

    Scratch_Arena scratch = scratch_begin(nullptr, 0);

    VkDescriptorSetLayout *global_layouts = push_array(scratch.arena,
        VkDescriptorSetLayout,
        context->swapchain.image_count);

    for (u32 i = 0; i < context->swapchain.image_count; ++i)
        global_layouts[i] = out_shader->global_descriptor_set_layout;

    VkDescriptorSetAllocateInfo alloc_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc_info.descriptorPool = out_shader->global_descriptor_pool;
    alloc_info.descriptorSetCount = context->swapchain.image_count;
    alloc_info.pSetLayouts = global_layouts;

    VK_CHECK(vkAllocateDescriptorSets(context->device.logical_device,
        &alloc_info,
        out_shader->global_descriptor_sets));

    scratch_end(scratch);

    // Fixed the device local and host visible at the same time bug by querying
    // the capabilities of the selected GPU and adjusting accordingly
    if (!vulkan_buffer_create(context,
            sizeof(Vulkan_Material_Shader_Instance_Ubo) *
                VULKAN_MAX_MATERIAL_COUNT,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            device_local_bits | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            true,
            &out_shader->object_uniform_buffer)) {

        CORE_ERROR("Material instance buffer creation failed for shader.");
        return false;
    }

    return true;
}

void vulkan_material_shader_pipeline_destroy(Vulkan_Context *context,
    Vulkan_Material_Shader_Pipeline *shader) {

    VkDevice logical_device = context->device.logical_device;

    // Destroy local object uniforms resources
    vkDestroyDescriptorPool(logical_device,
        shader->object_descriptor_pool,
        context->allocator);

    vkDestroyDescriptorSetLayout(logical_device,
        shader->object_descriptor_set_layout,
        context->allocator);

    // Destroy global uniforms resources
    vulkan_buffer_destroy(context, &shader->global_uniform_buffer);
    vulkan_buffer_destroy(context, &shader->object_uniform_buffer);

    vulkan_graphics_pipeline_destroy(context, &shader->pipeline);

    vkDestroyDescriptorPool(logical_device,
        shader->global_descriptor_pool,
        context->allocator);

    vkDestroyDescriptorSetLayout(logical_device,
        shader->global_descriptor_set_layout,
        context->allocator);

    for (u32 i = 0; i < VULKAN_MATERIAL_SHADER_STAGE_COUNT; ++i) {
        vkDestroyShaderModule(context->device.logical_device,
            shader->stages[i].handle,
            context->allocator);
    }
}

void vulkan_material_shader_pipeline_use(Vulkan_Context *context,
    Vulkan_Material_Shader_Pipeline *shader) {
    u32 image_index = context->image_index;
    vulkan_graphics_pipeline_bind(&context->command_buffers[image_index],
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        &shader->pipeline);
}

// Not all GPUs are capable of performing an update operation after a bind
// operation for the descriptor sets for instead I need to implement the updated
// pattern, because nertheless I need to just bind once
void vulkan_material_shader_pipeline_update_global_state(
    Vulkan_Context *context,
    Vulkan_Material_Shader_Pipeline *shader,
    f32 delta_time) {

    u32 image_index = context->image_index;

    VkCommandBuffer command_buffer =
        context->command_buffers[image_index].handle;

    VkDescriptorSet global_descriptor =
        shader->global_descriptor_sets[image_index];

    u32 range = sizeof(Vulkan_Material_Shader_Global_Ubo);
    u64 offset = sizeof(Vulkan_Material_Shader_Global_Ubo) * image_index;

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

void vulkan_material_shader_pipeline_set_model(Vulkan_Context *context,
    Vulkan_Material_Shader_Pipeline *shader,
    mat4 model) {

    if (context && shader) {
        u32 image_index = context->image_index;
        VkCommandBuffer command_buffer =
            context->command_buffers[image_index].handle;

        // Push constants work similar to unfiroms but they can work
        // without descriptor sets. It can be executed at any point
        // not necessarily inside a renderpass. Vulkan "has a limitation"
        // of 128 bytes for push constants
        vkCmdPushConstants(command_buffer,
            shader->pipeline.pipeline_layout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(mat4),
            &model);
    }
}

void vulkan_material_shader_pipeline_apply_material(Vulkan_Context *context,
    Vulkan_Material_Shader_Pipeline *shader,
    Material *material) {

    if (!context || !shader)
        return;

    u32 image_index = context->image_index;
    VkCommandBuffer command_buffer =
        context->command_buffers[image_index].handle;

    // Obtain material data
    Vulkan_Material_Shader_Object_State *object_state =
        &shader->object_states[material->internal_id];

    VkDescriptorSet object_descriptor_set =
        object_state->descriptor_sets[image_index];

    VkWriteDescriptorSet
        descriptor_writes[VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT];
    memory_zero(descriptor_writes,
        sizeof(VkWriteDescriptorSet) * VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT);

    u32 descriptor_count = 0;
    u32 descriptor_index = 0;

    // Descriptor 0 - Uniform buffer
    u32 range = sizeof(Vulkan_Material_Shader_Instance_Ubo);
    u32 offset =
        sizeof(Vulkan_Material_Shader_Instance_Ubo) * material->internal_id;
    Vulkan_Material_Shader_Instance_Ubo instance_ubo;

    instance_ubo.diffuse_color = material->diffuse_color;

    vulkan_buffer_load_data(context,
        &shader->object_uniform_buffer,
        offset,
        range,
        0,
        &instance_ubo);

    u32 *global_ubo_generation =
        &object_state->descriptor_states[descriptor_index]
             .generations[image_index];

    // Only apply this if the descirptor has not yet been updated
    if (*global_ubo_generation == INVALID_ID ||
        *global_ubo_generation != material->generation) {

        VkDescriptorBufferInfo buffer_info;
        buffer_info.buffer = shader->object_uniform_buffer.handle;
        buffer_info.offset = offset;
        buffer_info.range = range;

        VkWriteDescriptorSet descriptor = {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        descriptor.dstSet = object_descriptor_set;
        descriptor.dstBinding = descriptor_index;
        descriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor.descriptorCount = 1;
        descriptor.pBufferInfo = &buffer_info;

        descriptor_writes[descriptor_count] = descriptor;
        descriptor_count++;

        // Update the frame generation. In this case it is only needed once
        *global_ubo_generation = material->generation;
    }

    descriptor_index++;

    // Samplers
    constexpr const u32 sampler_count = 1;
    VkDescriptorImageInfo image_infos[1];
    for (u32 sampler_index = 0; sampler_index < sampler_count;
        ++sampler_index) {

        Texture_Type use = shader->sampler_uses[sampler_index];
        Texture *texture = nullptr;

        switch (use) {
        case Texture_Type::MAP_DIFFUSE:
            texture = material->diffuse_map.texture;
            break;
        default:
            CORE_FATAL("Unable to bind sampler to unknown use.");
            return;
        }

        u32 *descriptor_generation =
            &object_state->descriptor_states[descriptor_index]
                 .generations[image_index];

        u32 *descriptor_id =
            &object_state->descriptor_states[descriptor_index].ids[image_index];

        // If the texture hasn't been loaded yet, use the default texture
        if (texture->generation == INVALID_ID) {
            texture = texture_system_get_default_texture();

            // Reset the descriptor generation ID when using default texture
            *descriptor_generation = INVALID_ID;
        }

        if (texture && (*descriptor_id != texture->id ||
                           *descriptor_generation != texture->generation ||
                           *descriptor_generation == INVALID_ID)) {

            Vulkan_Texture_Data *internal_data =
                static_cast<Vulkan_Texture_Data *>(texture->internal_data);
            image_infos[sampler_index].imageLayout =
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            image_infos[sampler_index].imageView = internal_data->image.view;
            image_infos[sampler_index].sampler = internal_data->sampler;

            VkWriteDescriptorSet descriptor = {
                VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            descriptor.dstSet = object_descriptor_set;
            descriptor.dstBinding = descriptor_index;
            descriptor.descriptorType =
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor.descriptorCount = 1;
            descriptor.pImageInfo = &image_infos[sampler_index];

            descriptor_writes[descriptor_count] = descriptor;
            descriptor_count++;

            if (texture->generation != INVALID_ID) {
                *descriptor_generation = texture->generation;
                *descriptor_id = texture->id;
            }
        }
        descriptor_index++;
    }

    if (descriptor_count > 0) {
        vkUpdateDescriptorSets(context->device.logical_device,
            descriptor_count,
            descriptor_writes,
            0,
            nullptr);
    }

    // Bind the descript set to be updated
    vkCmdBindDescriptorSets(command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        shader->pipeline.pipeline_layout,
        1,
        1,
        &object_descriptor_set,
        0,
        0);
}

b8 vulkan_material_shader_pipeline_acquire_resource(Vulkan_Context *context,
    Vulkan_Material_Shader_Pipeline *shader,
    Material *material) {
    // TODO: Change the memory management inside the gpu buffer to freelist
    material->internal_id = shader->object_uniform_buffer_index;
    shader->object_uniform_buffer_index++;

    Vulkan_Material_Shader_Object_State *object_state =
        &shader->object_states[material->internal_id];

    for (u32 i = 0; i < VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT; ++i) {
        // Loop over the number of descriptor sets per object per image
        for (u32 j = 0; j < 3; ++j) {
            object_state->descriptor_states[i].generations[j] = INVALID_ID;
            object_state->descriptor_states[i].ids[j] = INVALID_ID;
        }
    }

    // Allocate the descriptor sets for the object
    VkDescriptorSetLayout layouts[3] = {shader->object_descriptor_set_layout,
        shader->object_descriptor_set_layout,
        shader->object_descriptor_set_layout};

    VkDescriptorSetAllocateInfo alloc_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};

    alloc_info.descriptorPool = shader->object_descriptor_pool;
    alloc_info.descriptorSetCount = 3; // One per image
    alloc_info.pSetLayouts = layouts;

    VkResult result = vkAllocateDescriptorSets(context->device.logical_device,
        &alloc_info,
        object_state->descriptor_sets);

    if (result != VK_SUCCESS) {
        CORE_ERROR("Error allocating descriptor sets in shader!");
        return false;
    }

    return true;
}

void vulkan_material_shader_pipeline_release_resource(Vulkan_Context *context,
    Vulkan_Material_Shader_Pipeline *shader,
    Material *material) {

    constexpr u32 descriptor_set_count = 3;

    vkDeviceWaitIdle(context->device.logical_device);

    VkResult result = vkFreeDescriptorSets(context->device.logical_device,
        shader->object_descriptor_pool,
        descriptor_set_count,
        shader->object_states[material->internal_id].descriptor_sets);

    for (u32 i = 0; i < VULKAN_MATERIAL_SHADER_DESCRIPTOR_COUNT; ++i) {
        for (u32 j = 0; j < 3; ++j) {
            shader->object_states[material->internal_id]
                .descriptor_states[i]
                .generations[j] = INVALID_ID;
        }
    }

    material->internal_id = INVALID_ID;
}
