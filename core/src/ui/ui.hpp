#pragma once

#include "ui_themes.hpp"
#include "ui_types.hpp"

enum class Font_Style : u8 { NORMAL, ITALIC, BOLD, BOLD_ITALIC, MAX_COUNT };

VOLTRUM_API b8 ui_initialize(UI_Layer* layers,
    u32 layer_count,
    UI_Theme theme,
    PFN_menu_callback menu_callback,
    const char* app_name,
    struct SDL_Window* window);

VOLTRUM_API void ui_shutdown(UI_Layer* layers, u32 layer_count);

VOLTRUM_API void ui_update_layers(UI_Layer* layers, u32 layer_count, f32 delta_t);

VOLTRUM_API struct ImDrawData*
ui_draw_layers(UI_Layer* layers, u32 layer_count, f32 delta_t);
