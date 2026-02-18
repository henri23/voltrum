#pragma once

#include "defines.hpp"

void viewport_component_on_attach(struct Editor_Layer_State *state);

void viewport_component_on_update(struct Editor_Layer_State *state,
                                  struct Frame_Context      *ctx);

void viewport_component_on_render(struct Editor_Layer_State  *state,
                                  struct Global_Client_State *global_state,
                                  f32                         delta_time);

b8 viewport_component_on_mouse_wheel(struct Editor_Layer_State *state,
                                     const struct Event        *event);

b8 viewport_component_on_mouse_moved(struct Editor_Layer_State *state,
                                     const struct Event        *event);
