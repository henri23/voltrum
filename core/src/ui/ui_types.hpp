#pragma once

#include "data_structures/dynamic_array.hpp"
#include "defines.hpp"

#include "ui_themes.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <implot.h>

using PFN_menu_callback = void (*)();

enum class Font_Style : u8
{
    NORMAL,
    ITALIC,
    BOLD,
    BOLD_ITALIC,
    MAX_COUNT
};

internal_var constexpr u8 FONT_MAX_COUNT = (u8)Font_Style::MAX_COUNT;

struct UI_Titlebar_State
{
    const char *title_text;

    struct Texture *app_icon_texture;
    struct Texture *minimize_icon_texture;
    struct Texture *maximize_icon_texture;
    struct Texture *restore_icon_texture;
    struct Texture *close_icon_texture;

    ImVec2 titlebar_min;
    ImVec2 titlebar_max;

    b8 is_titlebar_hovered;
    b8 is_menu_hovered;
};

struct UI_Dockspace_State
{
    u32 dockspace_id;
    b8  dockspace_open;
    b8  window_began;
};

struct UI_Layer
{
    void *state; // Layer state used in client, but managed in core

    void (*on_attach)(void *state);
    void (*on_detach)(void *state);

    b8 (*on_update)(void *state, f32 delta_time);
    b8 (*on_render)(void *state, f32 delta_time);

    // TODO: Enable later
    // b8 (*on_event)(void *state, Event event);
};

struct UI_State
{
    PFN_menu_callback menu_callback;
    UI_Theme          current_theme;
    const char       *app_name;
    b8                is_initialized;
    ImFont           *fonts[FONT_MAX_COUNT];

    Dynamic_Array<UI_Layer> *layers;
    UI_Titlebar_State        titlebar;
    UI_Dockspace_State       dockspace;
    struct Platform_State   *platform;
};
