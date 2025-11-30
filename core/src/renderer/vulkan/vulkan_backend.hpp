#pragma once

#include "renderer/renderer_types.hpp"
#include "resources/resource_types.hpp"
#include "vulkan_types.hpp"

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

void vulkan_draw_geometry(Geometry_Render_Data data);

// Get main renderer texture for ImGui display
VkDescriptorSet vulkan_get_main_texture();

// Resize main renderer target
void vulkan_resize_main_target(u32 width, u32 height);

void vulkan_create_texture(const u8* pixels, Texture* texture);

void vulkan_destroy_texture(Texture* texture);

b8 vulkan_create_material(struct Material* material);

void vulkan_destroy_material(struct Material* material);

b8 vulkan_create_geometry(Geometry* geometry,
    u32 vertex_count,
    const vertex_3d* vertices,
    u32 index_count,
    const u32* indices);

void vulkan_destroy_geometry(Geometry* geometry);
