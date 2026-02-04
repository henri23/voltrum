#pragma once

#include "renderer/renderer_types.hpp"

struct Renderer_System_State;

Renderer_System_State *renderer_init(
    Arena                 *allocator,
    struct Platform_State *platform,
    String                 application_name);

void renderer_on_resize(u16 width, u16 height);

b8 renderer_draw_frame(struct Frame_Context *frame_ctx, Render_Context *packet);

void renderer_create_texture(const u8       *pixels,
                             struct Texture *texture,
                             b8              is_ui_texture = false);
void renderer_destroy_texture(struct Texture *texture);

void *renderer_get_texture_draw_data(struct Texture *texture);

b8   renderer_create_material(struct Material *material);
void renderer_destroy_material(struct Material *material);

b8   renderer_create_geometry(Geometry        *geometry,
                              u32              vertex_count,
                              const vertex_3d *vertices,
                              u32              index_count,
                              u32             *indices);
void renderer_destroy_geometry(Geometry *geometry);

// WARN: The exposing of this method from the core library is temporary until
// the camera system is developed
VOLTRUM_API void renderer_set_view(mat4 view);

// Viewport management for editor
VOLTRUM_API void  renderer_render_viewport();
VOLTRUM_API void *renderer_get_rendered_viewport();
VOLTRUM_API void  renderer_resize_viewport(u32 width, u32 height);
VOLTRUM_API void  renderer_get_viewport_size(u32 *width, u32 *height);
