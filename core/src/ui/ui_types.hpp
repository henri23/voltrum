#pragma once

#include "data_structures/dynamic_array.hpp"
#include "defines.hpp"

#include "ui_themes.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <implot.h>

// Content bounds for the client-provided titlebar content area
struct Titlebar_Content_Bounds
{
    f32 x;      // Left edge of content area (after logo)
    f32 y;      // Top of titlebar
    f32 width;  // Available width (before window buttons)
    f32 height; // Full titlebar height
};

// Callback type for titlebar content rendering
using PFN_titlebar_content_callback = void (*)(
    void                          *client_state,
    const Titlebar_Content_Bounds &bounds,
    const UI_Theme_Palette        &palette
);

enum class Font_Style : u8
{
    NORMAL,
    ITALIC,
    BOLD,
    BOLD_ITALIC,
    MAX_COUNT
};

internal_var constexpr u8 FONT_MAX_COUNT = (u8)Font_Style::MAX_COUNT;

// Scale the ui for apple because retina display is not behaving well and seems
// to zoom everything, both the font and the titlebar icons
#ifdef PLATFORM_APPLE
internal_var constexpr f32 UI_PLATFORM_SCALE = 0.85f;
#else
internal_var constexpr f32 UI_PLATFORM_SCALE = 1.0f;
#endif

struct UI_Titlebar_State
{
    struct Texture *app_icon_texture;
    struct Texture *minimize_icon_texture;
    struct Texture *maximize_icon_texture;
    struct Texture *restore_icon_texture;
    struct Texture *close_icon_texture;

    ImVec2 titlebar_min;
    ImVec2 titlebar_max;

    // Button regions for SDL hit test exclusion (screen coordinates)
    ImVec2 button_area_min;
    ImVec2 button_area_max;

    // Content area for client callback
    Titlebar_Content_Bounds content_bounds;

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

    b8 (*on_update)(void                 *state,
                    void                 *global_state,
                    struct Frame_Context *context);

    b8 (*on_render)(void                 *state,
                    void                 *global_state,
                    struct Frame_Context *context);

    // TODO: Enable later
    // b8 (*on_event)(void *state, Event event);
};

struct UI_State
{
    PFN_titlebar_content_callback titlebar_content_callback;
    UI_Theme                      current_theme;
    const char                   *app_name;
    const char                   *logo_asset_name;
    b8                            is_initialized;
    ImFont                       *fonts[FONT_MAX_COUNT];

    Dynamic_Array<UI_Layer> *layers;
    UI_Titlebar_State        titlebar;
    UI_Dockspace_State       dockspace;
    struct Platform_State   *platform;

    void *global_client_state;
};
