#pragma once

#include "defines.hpp"
#include "math/math_types.hpp"
#include "ui/ui_types.hpp"

struct Viewport_Camera
{
    vec3 position;
    vec3 euler_angles;
    mat4 view_matrix;
    mat4 camera_matrix;
    b8   view_dirty;
};

struct Editor_Layer_State
{
    Viewport_Camera camera;
    b8              viewport_focused;
    b8              viewport_hovered;
    vec2            viewport_size;
    vec2            last_viewport_size;

    // Metrics tracking
    f32 fps;
    f32 frame_time_ms;
    f32 fps_accumulator;
    u32 fps_frame_count;

    // Demo windows
    b8 show_demo_window;
    b8 show_implot_demo_window;

    // Signal analyzer panel
    b8  show_signal_analyzer;
    f32 signal_time;
};

void editor_layer_on_attach(void *state);
void editor_layer_on_detach(void *state);
b8   editor_layer_on_update(void *state, f32 delta_time);
b8   editor_layer_on_render(void *state, f32 delta_time);

UI_Layer create_editor_layer(Editor_Layer_State *state);

// Demo window control - accessible from menu callback
void editor_toggle_demo_window();
b8   editor_is_demo_window_visible();

// ImPlot demo window control
void editor_toggle_implot_demo_window();
b8   editor_is_implot_demo_window_visible();

// Signal analyzer panel control
void editor_toggle_signal_analyzer();
b8   editor_is_signal_analyzer_visible();
