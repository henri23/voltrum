#pragma once

#include "defines.hpp"
#include "math/math_types.hpp"
#include "ui/ui_types.hpp"

void editor_layer_on_attach(UI_Layer* self);
void editor_layer_on_detach(UI_Layer* self);
b8 editor_layer_on_update(UI_Layer* self, f32 delta_time);
b8 editor_layer_on_render(UI_Layer* self, f32 delta_time);

UI_Layer create_editor_layer();

// Demo window control - accessible from menu callback
void editor_toggle_demo_window();
b8 editor_is_demo_window_visible();
