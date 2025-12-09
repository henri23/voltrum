#include "vulkan_pipeline.hpp"
#include "math/math_types.hpp"
#include "vulkan_utils.hpp"

#include "core/logger.hpp"

b8 vulkan_graphics_pipeline_create(Vulkan_Context* context,
    Vulkan_Renderpass* renderpass,
    u32 stride,
    u32 attribute_count,
    VkVertexInputAttributeDescription* attributes,
    u32 descriptor_set_layout_count,
    VkDescriptorSetLayout* descriptor_set_layouts,
    u32 stage_count,
    VkPipelineShaderStageCreateInfo* stages,
    VkViewport viewport,
    VkRect2D scissor,
    b8 is_wireframe,
    b8 depth_test_enabled,
    Vulkan_Pipeline* out_pipeline) {

    // Viewport state creation
    VkPipelineViewportStateCreateInfo viewport_state = {
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};

    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};

    // If depthClampEnable is set to VK_TRUE, the fragments beyond the near/far
    // planes are clamped rather than discarted
    rasterizer_create_info.depthClampEnable = VK_FALSE;

    // If rasterizerDiscardEnable is set to VK_TRUE, geometry never reaches the
    // rasterizer stage (useful for transform feedback)
    rasterizer_create_info.rasterizerDiscardEnable = VK_FALSE;

    // Specifies whether to fill the polygons or render as wireframe
    rasterizer_create_info.polygonMode =
        is_wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;

    // Disable backface culling for now to ensure triangle is visible regardless
    // of winding order
    rasterizer_create_info.cullMode = VK_CULL_MODE_NONE;

    // Decides what is considered the front face of a triangle
    rasterizer_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    // Disables depth bias. Depth bias is used to tweak depth values to avoid
    // z-fighting. typically for decals or shadow mapping
    rasterizer_create_info.depthBiasEnable = VK_FALSE;

    // Since no bias is applied, I leave these parameters zerod out
    rasterizer_create_info.depthBiasConstantFactor = 0.0f;
    rasterizer_create_info.depthBiasClamp = 0.0f;
    rasterizer_create_info.depthBiasSlopeFactor = 0.0f;

    // Multisampling - Not used for now so set to defaults
    VkPipelineMultisampleStateCreateInfo multisampling_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};

    multisampling_create_info.sampleShadingEnable = VK_FALSE;
    multisampling_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling_create_info.minSampleShading = 1.0f;
    multisampling_create_info.pSampleMask = 0;
    multisampling_create_info.alphaToCoverageEnable = VK_FALSE;
    multisampling_create_info.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depth_stencil = {
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    // Depth and stencil testing
    if (depth_test_enabled) {
        depth_stencil.depthTestEnable = VK_TRUE;
        depth_stencil.depthWriteEnable = VK_TRUE;
        depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depth_stencil.depthBoundsTestEnable = VK_FALSE;
        depth_stencil.stencilTestEnable = VK_FALSE;
    }

    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
    color_blend_attachment_state.blendEnable = VK_TRUE;
    color_blend_attachment_state.srcColorBlendFactor =
        VK_BLEND_FACTOR_SRC_ALPHA;

    color_blend_attachment_state.dstColorBlendFactor =
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

    color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.srcAlphaBlendFactor =
        VK_BLEND_FACTOR_SRC_ALPHA;

    color_blend_attachment_state.dstAlphaBlendFactor =
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

    color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;

    color_blend_attachment_state.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo color_blend_state_crate_info = {
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    color_blend_state_crate_info.logicOpEnable = VK_FALSE;
    color_blend_state_crate_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_crate_info.attachmentCount = 1;
    color_blend_state_crate_info.pAttachments = &color_blend_attachment_state;

    // Pipelines are immutable, however there are some "things" that are in
    // fact mutable. The properties in the dynamics states can be changed
    // without needing to recreate the whole pipeline
    constexpr u32 dynamic_state_count = 3;
    VkDynamicState dynamic_states[dynamic_state_count] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH};

    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic_state_create_info.dynamicStateCount = dynamic_state_count;
    dynamic_state_create_info.pDynamicStates = dynamic_states;

    VkVertexInputBindingDescription binding_description;
    binding_description.binding = 0;
    binding_description.stride = stride;
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_info.vertexAttributeDescriptionCount = attribute_count;
    vertex_input_info.pVertexAttributeDescriptions = attributes;

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

    // Push contants
    VkPushConstantRange push_constant;
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_constant.offset = sizeof(mat4) * 0;
    push_constant.size = sizeof(mat4) * 2; // 128 bytes

    pipeline_layout_create_info.pushConstantRangeCount = 1;
    pipeline_layout_create_info.pPushConstantRanges = &push_constant;

    // Descriptor set layouts
    pipeline_layout_create_info.setLayoutCount = descriptor_set_layout_count;
    pipeline_layout_create_info.pSetLayouts = descriptor_set_layouts;

    VK_CHECK(vkCreatePipelineLayout(context->device.logical_device,
        &pipeline_layout_create_info,
        context->allocator,
        &out_pipeline->pipeline_layout));

    VkGraphicsPipelineCreateInfo pipeline_create_info = {
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipeline_create_info.stageCount = stage_count;
    pipeline_create_info.pStages = stages;
    pipeline_create_info.pVertexInputState = &vertex_input_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly;

    pipeline_create_info.pViewportState = &viewport_state;
    pipeline_create_info.pRasterizationState = &rasterizer_create_info;
    pipeline_create_info.pMultisampleState = &multisampling_create_info;
    pipeline_create_info.pDepthStencilState =
        depth_test_enabled ? &depth_stencil : nullptr;

    pipeline_create_info.pColorBlendState = &color_blend_state_crate_info;
    pipeline_create_info.pDynamicState = &dynamic_state_create_info;
    pipeline_create_info.pTessellationState = 0;

    pipeline_create_info.layout = out_pipeline->pipeline_layout;

    pipeline_create_info.renderPass = renderpass->handle;
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_create_info.basePipelineIndex = -1;

    VkResult result = vkCreateGraphicsPipelines(context->device.logical_device,
        VK_NULL_HANDLE,
        1,
        &pipeline_create_info,
        context->allocator,
        &out_pipeline->handle);

    if (vulkan_result_is_success(result)) {
        CORE_DEBUG("Graphics pipeline created!");
        return true;
    }

    CORE_ERROR("vkCreateGraphicsPipelines failed with %s.",
        vulkan_result_string(result, true));

    return false;
}

void vulkan_graphics_pipeline_destroy(Vulkan_Context* context,
    Vulkan_Pipeline* pipeline) {

    if (pipeline) {
        if (pipeline->handle) {
            vkDestroyPipeline(context->device.logical_device,
                pipeline->handle,
                context->allocator);
            pipeline->handle = nullptr;
        }

        if (pipeline->pipeline_layout) {
            vkDestroyPipelineLayout(context->device.logical_device,
                pipeline->pipeline_layout,
                context->allocator);
            pipeline->pipeline_layout = nullptr;
        }
    }
}

void vulkan_graphics_pipeline_bind(Vulkan_Command_Buffer* command_buffer,
    VkPipelineBindPoint bind_point,
    Vulkan_Pipeline* pipeline) {
    vkCmdBindPipeline(command_buffer->handle, bind_point, pipeline->handle);
}
