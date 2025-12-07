#pragma once

#include "defines.hpp"
#include "renderer/vulkan/vulkan_types.hpp"

// Initialize ImGui context and backends
b8 vulkan_ui_backend_initialize(Vulkan_Context* context, void* window);

// Shutdown ImGui and clean up resources
void vulkan_ui_backend_shutdown(Vulkan_Context* context);

// Render ImGui draw data
void vulkan_ui_backend_render(Vulkan_Context* context,
    VkCommandBuffer command_buffer);
