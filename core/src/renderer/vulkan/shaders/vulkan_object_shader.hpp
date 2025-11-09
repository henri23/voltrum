#pragma once

#include "math/math_types.hpp"
#include "renderer/vulkan/vulkan_types.hpp"

b8 vulkan_object_shader_create(Vulkan_Context* context,
    Vulkan_Object_Shader* out_shader);

void vulkan_object_shader_destroy(Vulkan_Context* context,
    Vulkan_Object_Shader* shader);

void vulkan_object_shader_use(Vulkan_Context* context,
    Vulkan_Object_Shader* shader);

void vulkan_object_shader_update_global_state(Vulkan_Context* context,
    Vulkan_Object_Shader* shader);

void vulkan_object_shader_update_object(Vulkan_Context* context,
    Vulkan_Object_Shader* shader,
    mat4 model);
