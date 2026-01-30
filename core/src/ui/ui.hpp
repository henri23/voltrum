#pragma once

#include "data_structures/dynamic_array.hpp"
#include "memory/arena.hpp"
#include "ui_themes.hpp"
#include "ui_types.hpp"

VOLTRUM_API UI_State *ui_init(Arena                   *allocator,
                              Dynamic_Array<UI_Layer> *layers,
                              UI_Theme                 theme,
                              PFN_menu_callback        menu_callback,
                              const char              *app_name,
                              struct Platform_State   *plat_state);

VOLTRUM_API void ui_shutdown_layers(UI_State *state);

VOLTRUM_API void ui_update_layers(UI_State *state, f32 delta_t);

VOLTRUM_API struct ImDrawData *ui_draw_layers(UI_State *state, f32 delta_t);
