#pragma once

#include "vulkan_types.hpp"

void vulkan_swapchain_create(Vulkan_Context *context,
    u32 width,
    u32 height,
    Vulkan_Swapchain *out_swapchain);

void vulkan_swapchain_recreate(Vulkan_Context *context,
    u32 width,
    u32 height,
    Vulkan_Swapchain *out_swapchain);

void vulkan_swapchain_destroy(Vulkan_Context *context,
    Vulkan_Swapchain *swapchain);
