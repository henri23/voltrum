#pragma once

#ifdef DEBUG_BUILD

#    include "defines.hpp"
#    include "ui/ui_types.hpp"

struct Debug_Layer_State
{
    s32 selected_arena_index;
    f32 zoom_level;
    f32 scroll_x;
};

void debug_layer_on_attach(void *state);
void debug_layer_on_detach(void *state);
b8   debug_layer_on_update(void *state, Frame_Context *context);
b8   debug_layer_on_render(void *state, Frame_Context *context);

UI_Layer create_debug_layer(Debug_Layer_State *state);

void debug_toggle_layer();
b8   debug_is_layer_visible();

#endif // DEBUG_BUILD
