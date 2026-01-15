#pragma once

#include "vulkan_types.hpp"

void vulkan_viewport_create(Vulkan_Context *context,
    u32 width,
    u32 height,
    Vulkan_Viewport *out_viewport);

void vulkan_viewport_destroy(Vulkan_Context *context,
    Vulkan_Viewport *viewport);
