#pragma once

#include "renderer/renderer_types.hpp"

struct Static_Mesh_Data;

b8 renderer_startup(const char* application_name);

void renderer_shutdown();

void renderer_on_resize(u16 width, u16 height);

b8 renderer_begin_frame(f32 delta_t);

b8 renderer_end_frame(f32 delta_t);

b8 renderer_draw_frame(Render_Packet* packet);

// UI Image Resource Structure - directly exposes what users need
struct UI_Image_Resource {
    u32 handle;
    u32 width;
    u32 height;
    void* descriptor_set; // VkDescriptorSet cast to void* for abstraction
    b8 is_valid;
};

// UI Image Management
b8 renderer_create_ui_image(u32 width,
    u32 height,
    const void* pixel_data,
    u32 pixel_data_size,
    UI_Image_Resource* out_image_resource);

void renderer_destroy_ui_image(UI_Image_Resource* resource);

// WARN: The exposing of this method from the core library is temporary until
// the camera system is developed
VOLTRUM_API void renderer_set_view(mat4 view);
