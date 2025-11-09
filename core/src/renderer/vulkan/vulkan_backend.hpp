#pragma once

#include "renderer/renderer_types.hpp"
#include <vulkan/vulkan.h>

b8 vulkan_initialize(Renderer_Backend* backend, const char* app_name);

void vulkan_shutdown(Renderer_Backend* backend);

void vulkan_on_resized(Renderer_Backend* backend, u16 width, u16 height);

b8 vulkan_begin_frame(Renderer_Backend* backend, f32 delta_t);

b8 vulkan_end_frame(Renderer_Backend* backend, f32 delta_t);

void vulkan_update_global_state(mat4 projection,
    mat4 view,
    vec3 view_position,
    vec4 ambient_colour,
    s32 mode);

void vulkan_update_object(mat4 model);

// Get main renderer texture for ImGui display
VkDescriptorSet vulkan_get_main_texture();

// Resize main renderer target
void vulkan_resize_main_target(u32 width, u32 height);

// Forward declaration
struct UI_Image_Resource;

// UI Image Management - Renderer Backend Interface
b8 vulkan_create_ui_image(Renderer_Backend* backend,
    u32 width,
    u32 height,
    const void* pixel_data,
    u32 pixel_data_size,
    UI_Image_Resource* out_image_resource);

void vulkan_destroy_ui_image(Renderer_Backend* backend,
    UI_Image_Resource* resource);
