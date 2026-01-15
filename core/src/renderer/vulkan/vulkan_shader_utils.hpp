#pragma once

#include "vulkan_types.hpp"

b8 create_shader_module(Vulkan_Context *context,
    const char *name,
    const char *type_str,
    VkShaderStageFlagBits shader_stage_flags,
    u32 stage_index,
    Vulkan_Shader_Stage *shader_stages);
