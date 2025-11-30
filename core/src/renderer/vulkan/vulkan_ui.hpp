#pragma once

#include "renderer/vulkan/vulkan_types.hpp"

b8 ui_setup_vulkan_resources(Vulkan_Context* context);

void ui_cleanup_vulkan_resources(Vulkan_Context* context);

void ui_on_vulkan_resize(Vulkan_Context* context, u32 width, u32 height);

void ui_begin_vulkan_frame();

b8 ui_draw_components(Vulkan_Command_Buffer* command_buffer);
