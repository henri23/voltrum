#pragma once

#include "defines.hpp"
#include "math/math_types.hpp"
#include "ui/ui_types.hpp"

struct Viewport_Camera {
    vec3 position;
    vec3 euler_angles;
    mat4 view_matrix;
    mat4 camera_matrix;
    b8 view_dirty;
};

struct Editor_Layer_State {
    Viewport_Camera camera;
    b8 viewport_window_open;
    b8 viewport_focused;
    b8 viewport_hovered;
    vec2 viewport_size;
    vec2 last_viewport_size;
};

void editor_layer_on_attach(
    UI_Layer* self
);
void editor_layer_on_detach(
    UI_Layer* self
);
b8 editor_layer_on_update(
    UI_Layer* self,
    f32 delta_time
);
b8 editor_layer_on_render(
    UI_Layer* self,
    f32 delta_time
);

UI_Layer create_editor_layer();
