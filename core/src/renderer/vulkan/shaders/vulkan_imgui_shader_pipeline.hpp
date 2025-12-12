#pragma once

#include "renderer/vulkan/vulkan_types.hpp"

b8 vulkan_imgui_shader_pipeline_create(Vulkan_Context* context,
    Vulkan_ImGui_Shader_Pipeline* out_shader);

void vulkan_imgui_shader_pipeline_destroy(Vulkan_Context* context,
    Vulkan_ImGui_Shader_Pipeline* shader);

void vulkan_imgui_shader_pipeline_use(Vulkan_Context* context,
    Vulkan_ImGui_Shader_Pipeline* shader);

void vulkan_imgui_shader_pipeline_draw(Vulkan_Context* context,
    Vulkan_ImGui_Shader_Pipeline* shader,
    ImDrawData* draw_data);

VkDescriptorSet vulkan_imgui_shader_pipeline_create_texture_descriptor(
    Vulkan_Context* context,
    Vulkan_ImGui_Shader_Pipeline* shader,
    VkImageView image_view);

void vulkan_imgui_shader_pipeline_remove_texture_descriptor(
    VkDescriptorSet descriptor_set);

void vulkan_imgui_shader_pipeline_create_viewport_descriptors(
    Vulkan_Context* context,
    Vulkan_ImGui_Shader_Pipeline* shader);

void vulkan_imgui_shader_pipeline_destroy_viewport_descriptors(
    Vulkan_ImGui_Shader_Pipeline* shader);
