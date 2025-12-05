#pragma once

#include "renderer/vulkan/vulkan_types.hpp"

b8 vulkan_material_shader_pipeline_create(Vulkan_Context* context,
    Vulkan_Material_Shader_Pipeline* out_shader);

void vulkan_material_shader_pipeline_destroy(Vulkan_Context* context,
    Vulkan_Material_Shader_Pipeline* shader);

void vulkan_material_shader_pipeline_use(Vulkan_Context* context,
    Vulkan_Material_Shader_Pipeline* shader);

void vulkan_material_shader_pipeline_update_global_state(
    Vulkan_Context* context,
    Vulkan_Material_Shader_Pipeline* shader,
    f32 delta_time);

void vulkan_material_shader_pipeline_set_model(Vulkan_Context* context,
    Vulkan_Material_Shader_Pipeline* shader,
    mat4 model);

void vulkan_material_shader_pipeline_apply_material(Vulkan_Context* context,
    Vulkan_Material_Shader_Pipeline* shader,
    Material* material);

b8 vulkan_material_shader_pipeline_acquire_resource(Vulkan_Context* context,
    Vulkan_Material_Shader_Pipeline* shader,
    Material* material);

void vulkan_material_shader_pipeline_release_resource(Vulkan_Context* context,
    Vulkan_Material_Shader_Pipeline* shader,
    Material* material);
