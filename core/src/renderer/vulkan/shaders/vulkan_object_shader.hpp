#pragma once

#include "renderer/vulkan/vulkan_types.hpp"

b8 vulkan_object_shader_create(Vulkan_Context* context,
    Vulkan_Object_Shader* out_shader);

void vulkan_object_shader_destroy(Vulkan_Context* context,
    Vulkan_Object_Shader* shader);

void vulkan_object_shader_use(Vulkan_Context* context,
    Vulkan_Object_Shader* shader);

void vulkan_object_shader_update_global_state(Vulkan_Context* context,
    Vulkan_Object_Shader* shader,
    f32 delta_time);

void vulkan_object_shader_update_object(Vulkan_Context* context,
    Vulkan_Object_Shader* shader,
    Geometry_Render_Data data);

b8 vulkan_object_shader_acquire_resource(Vulkan_Context* context,
    Vulkan_Object_Shader* shader,
    Object_ID* out_object_id);

void vulkan_object_shader_release_resource(Vulkan_Context* context,
    Vulkan_Object_Shader* shader,
    Object_ID object_id);
