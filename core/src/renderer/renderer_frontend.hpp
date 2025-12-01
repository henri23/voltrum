#pragma once

#include "renderer/renderer_types.hpp"

struct Static_Mesh_Data;

b8 renderer_startup(const char* application_name);

void renderer_shutdown();

void renderer_on_resize(u16 width, u16 height);

b8 renderer_draw_frame(Render_Packet* packet);

void renderer_create_texture(const u8* pixels, struct Texture* texture);
void renderer_destroy_texture(struct Texture* texture);

b8 renderer_create_material(struct Material* material);
void renderer_destroy_material(struct Material* material);

b8 renderer_create_geometry(Geometry* geometry,
    u32 vertex_count,
    const vertex_3d* vertices,
    u32 index_count,
    u32* indices);
void renderer_destroy_geometry(Geometry* geometry);

// WARN: The exposing of this method from the core library is temporary until
// the camera system is developed
VOLTRUM_API void renderer_set_view(mat4 view);
