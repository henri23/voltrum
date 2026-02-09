#pragma once

#include "defines.hpp"
#include "ui/ui_types.hpp"

struct Utilities_Layer_State
{
    b8  toolbar_position_initialized;
    f32 toolbar_pos_x;
    f32 toolbar_pos_y;
    f32 toolbar_emphasis;
    s32 active_tool_index;
    struct Command_Palette_State *command_palette_state;
};

void utilities_layer_on_attach(void *state);
void utilities_layer_on_detach(void *state);
b8   utilities_layer_on_update(void                 *state,
                               void                 *global_state,
                               struct Frame_Context *ctx);
b8   utilities_layer_on_render(void                 *state,
                               void                 *global_state,
                               struct Frame_Context *ctx);

UI_Layer create_utilities_layer(Utilities_Layer_State *state);
