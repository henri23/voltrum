#pragma once

#include "defines.hpp"
#include "vulkan_types.hpp"

b8 vulkan_device_initialize(Vulkan_Context *context,
    Vulkan_Physical_Device_Requirements *requirements);

void vulkan_device_shutdown(Vulkan_Context *context);

void vulkan_device_query_swapchain_capabilities(VkPhysicalDevice device,
    VkSurfaceKHR surface,
    Vulkan_Swapchain_Support_Info *out_swapchain_info);

b8 vulkan_device_detect_depth_format(Vulkan_Device *device);
