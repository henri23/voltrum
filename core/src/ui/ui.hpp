#pragma once

#include "ui_themes.hpp"
#include "ui_types.hpp"

VOLTRUM_API b8 ui_initialize(UI_Context *context,
    UI_Layer *layers,
    u32 layer_count,
    UI_Theme theme,
    PFN_menu_callback menu_callback,
    const char *app_name,
    struct SDL_Window *window);

VOLTRUM_API void
ui_shutdown(UI_Context *context, UI_Layer *layers, u32 layer_count);

VOLTRUM_API void ui_update_layers(UI_Context *context,
    UI_Layer *layers,
    u32 layer_count,
    f32 delta_t);

VOLTRUM_API struct ImDrawData *ui_draw_layers(UI_Context *context,
    UI_Layer *layers,
    u32 layer_count,
    f32 delta_t);
