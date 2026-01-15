#pragma once

#include "vulkan_types.hpp"

const char *vulkan_result_string(VkResult result, b8 get_extended);

b8 vulkan_result_is_success(VkResult result);
