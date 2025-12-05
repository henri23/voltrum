#pragma once

#include "renderer/vulkan/vulkan_types.hpp"

b8 vulkan_imgui_shader_pipeline_create(
    Vulkan_Context* context,
    Vulkan_ImGui_Shader_Pipeline* out_shader
);

void vulkan_imgui_shader_pipeline_destroy(
    Vulkan_Context* context,
    Vulkan_ImGui_Shader_Pipeline* shader
);

void vulkan_imgui_shader_pipeline_use(
    Vulkan_Context* context,
    Vulkan_ImGui_Shader_Pipeline* shader
);
