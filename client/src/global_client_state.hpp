#pragma once

#include "defines.hpp"

// Client-specific state structure
struct Global_Client_State
{
    b8 initialized;

    // Statuses of layers
    b8 is_debug_layer_visible;

    // Statuses of windows
    b8 is_imgui_demo_visible;
    b8 is_implot_demo_visible;

    // Titlebar interaction state
    b8  is_titlebar_menu_expanded;
    s32 titlebar_active_mode_index;
    f32 titlebar_mode_anim_t;
    f32 titlebar_menu_overlay_t;
    f32 titlebar_menu_hover_open_t;
};
