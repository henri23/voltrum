#pragma once

#include "defines.hpp"
#include "math/math_types.hpp"
#include "ui/ui_types.hpp"

struct Viewport_Camera_2D
{
    vec2 position;    // World-space center of the view
    f32  zoom;        // Current pixels per world unit (animated)
    f32  target_zoom; // Target zoom level for smooth animation
    b8   zoom_anchor_active;
    vec2 zoom_anchor_viewport_local;
    vec2 zoom_anchor_world;
    b8   dirty;
};

#ifdef DEBUG_BUILD
struct Viewport_Camera_3D_Debug
{
    b8   enabled;
    vec3 position;
    f32  yaw_degrees;
    f32  pitch_degrees;
    f32  move_speed;
    b8   orbit_active;
    vec3 orbit_pivot;
};
#endif

struct Editor_Layer_State
{
    Viewport_Camera_2D camera;
#ifdef DEBUG_BUILD
    Viewport_Camera_3D_Debug debug_camera;
#endif
    b8                 viewport_focused;
    b8                 viewport_hovered;
    vec2               viewport_size;
    vec2               last_viewport_size;
    vec2               viewport_image_pos;
    vec2               viewport_image_size;
    b8                 cursor_world_valid;
    vec2               cursor_world_position;
    f32                grid_spacing;

    // Metrics tracking
    f32 fps;
    f32 frame_time_ms;
    f32 fps_accumulator;
    u32 fps_frame_count;

    // Signal analyzer panel
    f32 signal_time;
};

void editor_layer_on_attach(void *state);
void editor_layer_on_detach(void *state);
b8   editor_layer_on_update(void          *state,
                            void          *global_state,
                            struct Frame_Context *ctx);
b8   editor_layer_on_render(void          *state,
                            void          *global_state,
                            struct Frame_Context *ctx);

UI_Layer create_editor_layer(Editor_Layer_State *state);
