#pragma once

#include "renderer/vulkan/vulkan_types.hpp"

b8 vulkan_material_shader_create(Vulkan_Context* context,
    Vulkan_Material_Shader* out_shader);

void vulkan_material_shader_destroy(Vulkan_Context* context,
    Vulkan_Material_Shader* shader);

void vulkan_material_shader_use(Vulkan_Context* context,
    Vulkan_Material_Shader* shader);

void vulkan_material_shader_update_global_state(Vulkan_Context* context,
    Vulkan_Material_Shader* shader,
    f32 delta_time);

void vulkan_material_shader_set_model(Vulkan_Context* context,
    Vulkan_Material_Shader* shader,
    mat4 model);

void vulkan_material_shader_apply_material(Vulkan_Context* context,
    Vulkan_Material_Shader* shader,
    Material* material);

b8 vulkan_material_shader_acquire_resource(Vulkan_Context* context,
    Vulkan_Material_Shader* shader,
    Material* material);

void vulkan_material_shader_release_resource(Vulkan_Context* context,
    Vulkan_Material_Shader* shader,
    Material* material);
