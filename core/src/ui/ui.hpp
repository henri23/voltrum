#pragma once

#include "data_structures/auto_array.hpp"
#include "events/events.hpp"
#include "ui_themes.hpp"
#include "ui_types.hpp"

enum class Font_Style : u8 { NORMAL, ITALIC, BOLD, BOLD_ITALIC, MAX_COUNT };

// Forward declarations
struct ImDrawData;
struct ImFont;

VOLTRUM_API b8 ui_initialize(UI_Theme theme,
    Auto_Array<UI_Layer>* layers,
    PFN_menu_callback menu_callback,
    const char* app_name,
    void* window);

VOLTRUM_API void ui_shutdown();

VOLTRUM_API b8 ui_register_component(const UI_Layer* component);

UI_Theme ui_get_current_theme();

VOLTRUM_API void ui_change_theme(UI_Theme theme);

VOLTRUM_API PFN_event_callback ui_get_event_callback();

VOLTRUM_API void* ui_get_imgui_context();

// Used to retrieve font variations in the ui
VOLTRUM_API ImFont* ui_get_font(Font_Style style);

#define USE_FONT(style) ImGui::PushFont(ui_get_font(Font_Style::style));
#define END_FONT() ImGui::PopFont();
