#pragma once

#include "renderer/vulkan/vulkan_types.hpp"

b8 vulkan_grid_shader_pipeline_create(
    Vulkan_Context              *context,
    Vulkan_Grid_Shader_Pipeline *out_shader);

void vulkan_grid_shader_pipeline_destroy(
    Vulkan_Context              *context,
    Vulkan_Grid_Shader_Pipeline *shader);

void vulkan_grid_shader_pipeline_use(
    Vulkan_Context              *context,
    Vulkan_Grid_Shader_Pipeline *shader);

void vulkan_grid_shader_pipeline_update_global_state(
    Vulkan_Context              *context,
    Vulkan_Grid_Shader_Pipeline *shader);

void vulkan_grid_shader_pipeline_draw(
    Vulkan_Context              *context,
    Vulkan_Grid_Shader_Pipeline *shader);
