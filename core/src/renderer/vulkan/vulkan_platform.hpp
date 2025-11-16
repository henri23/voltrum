#pragma once

#include "data_structures/auto_array.hpp"
#include "renderer/vulkan/vulkan_types.hpp"

struct Platform_State;

void platform_get_required_extensions(
    Auto_Array<const char*>* required_extensions);

b8 platform_create_vulkan_surface(Vulkan_Context* context);
