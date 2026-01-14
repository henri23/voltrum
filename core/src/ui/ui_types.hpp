#pragma once

#include "defines.hpp"

#include "ui_themes.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

using PFN_menu_callback = void (*)();

enum class Font_Style : u8 { NORMAL, ITALIC, BOLD, BOLD_ITALIC, MAX_COUNT };

struct UI_Titlebar_State {
    const char* title_text;

    struct Texture* app_icon_texture;
    struct Texture* minimize_icon_texture;
    struct Texture* maximize_icon_texture;
    struct Texture* restore_icon_texture;
    struct Texture* close_icon_texture;

    ImVec2 titlebar_min;
    ImVec2 titlebar_max;

    b8 is_titlebar_hovered;
    b8 is_menu_hovered;
};

struct UI_Dockspace_State {
    unsigned int dockspace_id;
    b8 dockspace_open;
    b8 window_began;
};

struct UI_Context {
    UI_Theme current_theme;

    PFN_menu_callback menu_callback;
    const char* app_name;

    b8 is_initialized;

    ImFont* fonts[(u8)Font_Style::MAX_COUNT];

    UI_Titlebar_State titlebar;
    UI_Dockspace_State dockspace;
};

struct UI_Layer {
    void* state; // Layer state used in client, but managed in core

    void (*on_attach)(UI_Layer* self);
    void (*on_detach)(UI_Layer* self);

    b8 (*on_update)(UI_Layer* self, f32 delta_time);
    b8 (*on_render)(UI_Layer* self, f32 delta_time);
    // TODO: Enable later
    // b8 (*on_event)(UI_Layer* self, Event event);
};
