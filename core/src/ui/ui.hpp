#pragma once

#include "data_structures/dynamic_array.hpp"
#include "memory/arena.hpp"
#include "ui_themes.hpp"
#include "ui_types.hpp"
#include "utils/string.hpp"

VOLTRUM_API UI_State *ui_init(Arena                   *arena,
                              Dynamic_Array<UI_Layer> *layers,
                              UI_Theme                 theme,
                              PFN_menu_callback        menu_callback,
                              String                   app_name,
                              struct Platform_State   *plat_state,
                              void                    *global_client_state);

VOLTRUM_API void ui_shutdown_layers(UI_State *state);

VOLTRUM_API void ui_update_layers(UI_State             *state,
                                  struct Frame_Context *frame_ctx);

VOLTRUM_API struct ImDrawData *ui_draw_layers(UI_State             *state,
                                              struct Frame_Context *frame_ctx);
