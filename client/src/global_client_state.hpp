#pragma once

#include "defines.hpp"
#include <ui/ui_themes.hpp>

enum class Client_Mode
{
    SCHEMATIC,
    LAYOUT,
    SYMBOL,
    SIMULATION
};

// Client-specific state structure
struct Global_Client_State
{
    Client_Mode mode;

    // Statuses of layers
    b8 is_debug_layer_visible;

    // Statuses of windows
    b8 is_imgui_demo_visible;
    b8 is_implot_demo_visible;

    // Viewport bounds in screen space (for utility overlays/layers).
    b8  viewport_bounds_valid;
    f32 viewport_bounds_x;
    f32 viewport_bounds_y;
    f32 viewport_bounds_width;
    f32 viewport_bounds_height;

    // Command palette state
    b8 is_command_palette_open;
    b8 request_open_command_palette;
    b8 request_close_command_palette;

    // Theme state (authoritative source for client components)
    UI_Theme         target_theme;
    UI_Theme         requested_theme;
    b8               request_theme_change;
    b8               is_theme_transitioning;
    f32              theme_transition_t;
    UI_Theme_Palette theme_palette;
    UI_Theme_Palette theme_transition_from;
    UI_Theme_Palette theme_transition_to;
};
