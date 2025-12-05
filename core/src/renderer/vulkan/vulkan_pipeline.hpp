#pragma once

#include "vulkan_types.hpp"

b8 vulkan_graphics_pipeline_create(Vulkan_Context* context,
    // Does not use this renderpass necessarily, but it needs to use a render-
    // pass that uses the same setup and the renderpass passed here
    Vulkan_Renderpass* renderpass,
    u32 stride,
    u32 attribute_count,
    VkVertexInputAttributeDescription* attributes,
    u32 descriptor_set_layout_count,
    VkDescriptorSetLayout* scriptor_set_layouts,
    u32 stage_count,
    VkPipelineShaderStageCreateInfo* stages,
    VkViewport viewport,
    VkRect2D scissor,
    b8 is_wireframe,
    b8 depth_test_enabled,
    Vulkan_Pipeline* out_pipeline);

void vulkan_graphics_pipeline_destroy(Vulkan_Context* context,
    Vulkan_Pipeline* pipeline);

void vulkan_graphics_pipeline_bind(Vulkan_Command_Buffer* command_buffer,
    VkPipelineBindPoint bind_point,
    Vulkan_Pipeline* pipeline);
