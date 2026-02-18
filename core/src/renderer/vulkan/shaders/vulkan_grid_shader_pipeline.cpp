#include "vulkan_grid_shader_pipeline.hpp"
#include "defines.hpp"

#include "renderer/vulkan/vulkan_buffer.hpp"
#include "renderer/vulkan/vulkan_pipeline.hpp"
#include "renderer/vulkan/vulkan_types.hpp"

#include "core/logger.hpp"
#include "core/thread_context.hpp"
#include "memory/arena.hpp"
#include "renderer/vulkan/vulkan_shader_utils.hpp"
#include "renderer/vulkan/vulkan_utils.hpp"

#define BUILTIN_SHADER_NAME_GRID "Builtin.GridShader"

b8
vulkan_grid_shader_pipeline_create(
    Vulkan_Context              *context,
    Vulkan_Grid_Shader_Pipeline *out_shader)
{
    out_shader->global_ubo_stride = sizeof(Vulkan_Grid_Shader_Global_Ubo);
    u64 min_ubo_alignment =
        context->device.physical_device_properties.limits
            .minUniformBufferOffsetAlignment;
    if (min_ubo_alignment > 0)
    {
        out_shader->global_ubo_stride =
            ALIGN_UP(out_shader->global_ubo_stride, min_ubo_alignment);
    }

    char stage_type_strs[VULKAN_GRID_SHADER_STAGE_COUNT][5] = {"vert",
                                                                "frag"};

    VkShaderStageFlagBits state_types[VULKAN_GRID_SHADER_STAGE_COUNT] = {
        VK_SHADER_STAGE_VERTEX_BIT,
        VK_SHADER_STAGE_FRAGMENT_BIT};

    for (u32 i = 0; i < VULKAN_GRID_SHADER_STAGE_COUNT; ++i)
    {
        if (!create_shader_module(context,
                                  BUILTIN_SHADER_NAME_GRID,
                                  stage_type_strs[i],
                                  state_types[i],
                                  i,
                                  out_shader->stages))
        {

            CORE_ERROR("Failed to create %s shader module for '%s'",
                       stage_type_strs[i],
                       BUILTIN_SHADER_NAME_GRID);

            return false;
        }
    }

    // Global descriptors -- single UBO binding for both vertex and fragment
    VkDescriptorSetLayoutBinding global_ubo_layout_binding;
    global_ubo_layout_binding.binding         = 0;
    global_ubo_layout_binding.descriptorCount = 1;
    global_ubo_layout_binding.descriptorType =
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    global_ubo_layout_binding.pImmutableSamplers = 0;
    global_ubo_layout_binding.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo global_layout_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    global_layout_info.bindingCount = 1;
    global_layout_info.pBindings    = &global_ubo_layout_binding;

    VK_CHECK(
        vkCreateDescriptorSetLayout(context->device.logical_device,
                                    &global_layout_info,
                                    context->allocator,
                                    &out_shader->global_descriptor_set_layout));

    VkDescriptorPoolSize global_pool_size;
    global_pool_size.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    global_pool_size.descriptorCount = context->swapchain.image_count;

    VkDescriptorPoolCreateInfo global_pool_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    global_pool_info.poolSizeCount = 1;
    global_pool_info.pPoolSizes    = &global_pool_size;
    global_pool_info.maxSets       = context->swapchain.image_count;

    VK_CHECK(vkCreateDescriptorPool(context->device.logical_device,
                                    &global_pool_info,
                                    context->allocator,
                                    &out_shader->global_descriptor_pool));

    // Pipeline creation -- done directly since we have no vertex input
    // and no push constants (unlike vulkan_graphics_pipeline_create)
    VkViewport viewport;
    viewport.x        = 0.0f;
    viewport.y        = (f32)context->viewport.framebuffer_height;
    viewport.width    = (f32)context->viewport.framebuffer_width;
    viewport.height   = -(f32)context->viewport.framebuffer_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width                = context->viewport.framebuffer_width;
    scissor.extent.height               = context->viewport.framebuffer_height;

    VkPipelineViewportStateCreateInfo viewport_state = {
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport_state.viewportCount = 1;
    viewport_state.pViewports    = &viewport;
    viewport_state.scissorCount  = 1;
    viewport_state.pScissors     = &scissor;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizer_create_info.depthClampEnable        = VK_FALSE;
    rasterizer_create_info.rasterizerDiscardEnable  = VK_FALSE;
    rasterizer_create_info.polygonMode              = VK_POLYGON_MODE_FILL;
    rasterizer_create_info.cullMode                 = VK_CULL_MODE_NONE;
    rasterizer_create_info.frontFace                = VK_FRONT_FACE_CLOCKWISE;
    rasterizer_create_info.depthBiasEnable          = VK_FALSE;
    rasterizer_create_info.depthBiasConstantFactor  = 0.0f;
    rasterizer_create_info.depthBiasClamp           = 0.0f;
    rasterizer_create_info.depthBiasSlopeFactor     = 0.0f;
    rasterizer_create_info.lineWidth                = 1.0f;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling_create_info.sampleShadingEnable   = VK_FALSE;
    multisampling_create_info.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multisampling_create_info.minSampleShading      = 1.0f;
    multisampling_create_info.pSampleMask           = 0;
    multisampling_create_info.alphaToCoverageEnable = VK_FALSE;
    multisampling_create_info.alphaToOneEnable      = VK_FALSE;

    // Alpha blending
    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
    color_blend_attachment_state.blendEnable         = VK_TRUE;
    color_blend_attachment_state.srcColorBlendFactor  = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstColorBlendFactor  =
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment_state.colorBlendOp         = VK_BLEND_OP_ADD;
    color_blend_attachment_state.srcAlphaBlendFactor   = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment_state.dstAlphaBlendFactor   =
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment_state.alphaBlendOp          = VK_BLEND_OP_ADD;
    color_blend_attachment_state.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    color_blend_state_create_info.logicOpEnable   = VK_FALSE;
    color_blend_state_create_info.logicOp         = VK_LOGIC_OP_COPY;
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments    = &color_blend_attachment_state;

    // Dynamic states
    constexpr u32  dynamic_state_count                    = 3;
    VkDynamicState dynamic_states[dynamic_state_count]    = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH};

    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic_state_create_info.dynamicStateCount = dynamic_state_count;
    dynamic_state_create_info.pDynamicStates    = dynamic_states;

    // No vertex input -- fullscreen triangle via gl_VertexIndex
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertex_input_info.vertexBindingDescriptionCount   = 0;
    vertex_input_info.pVertexBindingDescriptions      = nullptr;
    vertex_input_info.vertexAttributeDescriptionCount = 0;
    vertex_input_info.pVertexAttributeDescriptions    = nullptr;

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    input_assembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable  = VK_FALSE;

    // Pipeline layout -- no push constants, single descriptor set layout
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges    = nullptr;
    pipeline_layout_create_info.setLayoutCount         = 1;
    pipeline_layout_create_info.pSetLayouts =
        &out_shader->global_descriptor_set_layout;

    VK_CHECK(
        vkCreatePipelineLayout(context->device.logical_device,
                               &pipeline_layout_create_info,
                               context->allocator,
                               &out_shader->pipeline.pipeline_layout));

    // Depth stencil -- disabled (grid is background layer)
    VkPipelineDepthStencilStateCreateInfo depth_stencil = {
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depth_stencil.depthTestEnable       = VK_FALSE;
    depth_stencil.depthWriteEnable      = VK_FALSE;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable     = VK_FALSE;

    // Shader stages
    VkPipelineShaderStageCreateInfo
        stage_create_infos[VULKAN_GRID_SHADER_STAGE_COUNT];

    memory_zero(stage_create_infos, sizeof(stage_create_infos));
    for (u32 i = 0; i < VULKAN_GRID_SHADER_STAGE_COUNT; ++i)
    {
        stage_create_infos[i] = out_shader->stages[i].shader_stage_create_info;
    }

    VkGraphicsPipelineCreateInfo pipeline_create_info = {
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipeline_create_info.stageCount          = VULKAN_GRID_SHADER_STAGE_COUNT;
    pipeline_create_info.pStages             = stage_create_infos;
    pipeline_create_info.pVertexInputState   = &vertex_input_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly;
    pipeline_create_info.pViewportState      = &viewport_state;
    pipeline_create_info.pRasterizationState = &rasterizer_create_info;
    pipeline_create_info.pMultisampleState   = &multisampling_create_info;
    pipeline_create_info.pDepthStencilState  = &depth_stencil;
    pipeline_create_info.pColorBlendState    = &color_blend_state_create_info;
    pipeline_create_info.pDynamicState       = &dynamic_state_create_info;
    pipeline_create_info.pTessellationState  = 0;

    pipeline_create_info.layout     = out_shader->pipeline.pipeline_layout;
    pipeline_create_info.renderPass = context->viewport_renderpass.handle;
    pipeline_create_info.subpass    = 0;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_create_info.basePipelineIndex  = -1;

    VkResult result =
        vkCreateGraphicsPipelines(context->device.logical_device,
                                  VK_NULL_HANDLE,
                                  1,
                                  &pipeline_create_info,
                                  context->allocator,
                                  &out_shader->pipeline.handle);

    if (!vulkan_result_is_success(result))
    {
        CORE_ERROR("vkCreateGraphicsPipelines failed for grid shader with %s.",
                   vulkan_result_string(result, true));
        return false;
    }

    // Create UBO buffer
    u32 device_local_bits = context->device.supports_device_local_host_visible
                                ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                                : 0;

    if (!vulkan_buffer_create(context,
                              out_shader->global_ubo_stride *
                                  context->swapchain.image_count,
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              device_local_bits |
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              true,
                              &out_shader->global_uniform_buffer))
    {

        CORE_ERROR("Vulkan buffer creation failed for grid shader.");
        return false;
    }

    // Allocate descriptor sets
    Scratch_Arena scratch = scratch_begin(nullptr, 0);

    VkDescriptorSetLayout *global_layouts =
        push_array(scratch.arena,
                   VkDescriptorSetLayout,
                   context->swapchain.image_count);

    for (u32 i = 0; i < context->swapchain.image_count; ++i)
        global_layouts[i] = out_shader->global_descriptor_set_layout;

    VkDescriptorSetAllocateInfo alloc_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc_info.descriptorPool     = out_shader->global_descriptor_pool;
    alloc_info.descriptorSetCount = context->swapchain.image_count;
    alloc_info.pSetLayouts        = global_layouts;

    VK_CHECK(vkAllocateDescriptorSets(context->device.logical_device,
                                      &alloc_info,
                                      out_shader->global_descriptor_sets));

    scratch_end(scratch);

    CORE_INFO("Grid shader pipeline created");
    return true;
}

void
vulkan_grid_shader_pipeline_destroy(
    Vulkan_Context              *context,
    Vulkan_Grid_Shader_Pipeline *shader)
{

    VkDevice logical_device = context->device.logical_device;

    vulkan_buffer_destroy(context, &shader->global_uniform_buffer);

    vulkan_graphics_pipeline_destroy(context, &shader->pipeline);

    vkDestroyDescriptorPool(logical_device,
                            shader->global_descriptor_pool,
                            context->allocator);

    vkDestroyDescriptorSetLayout(logical_device,
                                 shader->global_descriptor_set_layout,
                                 context->allocator);

    for (u32 i = 0; i < VULKAN_GRID_SHADER_STAGE_COUNT; ++i)
    {
        vkDestroyShaderModule(context->device.logical_device,
                              shader->stages[i].handle,
                              context->allocator);
    }
}

void
vulkan_grid_shader_pipeline_use(
    Vulkan_Context              *context,
    Vulkan_Grid_Shader_Pipeline *shader)
{
    u32 image_index = context->image_index;
    vulkan_graphics_pipeline_bind(&context->command_buffers[image_index],
                                  VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  &shader->pipeline);
}

void
vulkan_grid_shader_pipeline_update_global_state(
    Vulkan_Context              *context,
    Vulkan_Grid_Shader_Pipeline *shader)
{

    u32 image_index = context->image_index;

    VkCommandBuffer command_buffer =
        context->command_buffers[image_index].handle;

    VkDescriptorSet global_descriptor =
        shader->global_descriptor_sets[image_index];

    u32 range  = sizeof(Vulkan_Grid_Shader_Global_Ubo);
    u64 offset = shader->global_ubo_stride * image_index;

    vulkan_buffer_load_data(context,
                            &shader->global_uniform_buffer,
                            offset,
                            range,
                            0,
                            &shader->global_ubo);

    VkDescriptorBufferInfo bufferInfo;
    bufferInfo.buffer = shader->global_uniform_buffer.handle;
    bufferInfo.offset = offset;
    bufferInfo.range  = range;

    VkWriteDescriptorSet descriptor_write = {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptor_write.dstSet          = shader->global_descriptor_sets[image_index];
    descriptor_write.dstBinding      = 0;
    descriptor_write.dstArrayElement = 0;
    descriptor_write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_write.descriptorCount = 1;
    descriptor_write.pBufferInfo     = &bufferInfo;

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

void
vulkan_grid_shader_pipeline_draw(
    Vulkan_Context              *context,
    Vulkan_Grid_Shader_Pipeline *shader)
{
    u32             image_index    = context->image_index;
    VkCommandBuffer command_buffer = context->command_buffers[image_index].handle;

    // 3 vertices for fullscreen triangle, 1 instance
    vkCmdDraw(command_buffer, 3, 1, 0, 0);
}
