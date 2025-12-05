#pragma once

#include "renderer/vulkan/vulkan_types.hpp"
#include "defines.hpp"

// Initialize ImGui context and backends
b8 vulkan_ui_backend_initialize(
    Vulkan_Context* context,
    void* window);

// Shutdown ImGui and clean up resources
void vulkan_ui_backend_shutdown(Vulkan_Context* context);

// Start a new ImGui frame
void vulkan_ui_backend_new_frame();

// Render ImGui draw data
void vulkan_ui_backend_render(
    Vulkan_Context* context,
    VkCommandBuffer command_buffer);
