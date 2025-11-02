#pragma once

#include "defines.hpp"
#include "ui/ui_themes.hpp"
#include "imgui.h"

// Dark theme palette (current Walnut-inspired theme) - C++17 compatible
static const UI_Theme_Palette dark_palette = {
    // Primary colors
    IM_COL32(72,
        198,
        195,
        255), // accent - Professional teal/cyan
    IM_COL32(100,
        220,
        220,
        255), // highlight - Bright cyan
    IM_COL32(130,
        235,
        235,
        255), // nice_blue - Light cyan
    IM_COL32(90,
        180,
        180,
        255), // compliment - Darker teal

    // Background colors
    IM_COL32(38,
        42,
        48,
        255), // background
    IM_COL32(28,
        32,
        38,
        255), // background_dark
    IM_COL32(22,
        26,
        32,
        255), // titlebar - Dark blue-gray
    IM_COL32(22,
        26,
        32,
        255), // window_bg - Dark blue-gray
    IM_COL32(18,
        22,
        28,
        255), // property_field
    IM_COL32(45,
        50,
        58,
        255), // background_popup
    IM_COL32(15,
        18,
        24,
        255), // clear_color

    // Titlebar gradient colors
    IM_COL32(72,
        198,
        195,
        80), // titlebar_gradient_start - Teal with transparency
    IM_COL32(72,
        198,
        195,
        0), // titlebar_gradient_end - Teal fully transparent

    // Text colors
    IM_COL32(192,
        192,
        192,
        255), // text
    IM_COL32(210,
        210,
        210,
        255), // text_brighter
    IM_COL32(128,
        128,
        128,
        255), // text_darker
    IM_COL32(230,
        51,
        51,
        255), // text_error

    // UI element colors
    IM_COL32(65,
        72,
        82,
        255), // muted
    IM_COL32(48,
        54,
        62,
        255), // group_header
    IM_COL32(72,
        198,
        195,
        255), // selection - Teal (matches accent)
    IM_COL32(72,
        198,
        195,
        60), // selection_muted

    // Button colors
    IM_COL32(48,
        54,
        62,
        200), // button
    IM_COL32(65,
        72,
        82,
        255), // button_hovered
    IM_COL32(58,
        64,
        72,
        255), // button_active

    // Tab colors
    IM_COL32(72,
        198,
        195,
        80), // tab_hovered
    IM_COL32(72,
        198,
        195,
        120), // tab_active

    // Resize grip colors
    IM_COL32(232,
        232,
        232,
        64), // resize_grip (25% alpha)
    IM_COL32(207,
        207,
        207,
        171), // resize_grip_hovered (67% alpha)
    IM_COL32(117,
        117,
        117,
        242), // resize_grip_active (95% alpha)

    // Scrollbar colors
    IM_COL32(18,
        22,
        28,
        135), // scrollbar_bg
    IM_COL32(65,
        72,
        82,
        255), // scrollbar_grab
    IM_COL32(85,
        92,
        102,
        255), // scrollbar_grab_hovered
    IM_COL32(105,
        115,
        125,
        255), // scrollbar_grab_active

    // Separator colors
    IM_COL32(72,
        198,
        195,
        150), // separator_hovered

    // Docking colors
    IM_COL32(72,
        198,
        195,
        200), // docking_preview

    // Component colors for custom wrappers
    IM_COL32(72,
        198,
        195,
        255), // component_primary - Teal accent
    IM_COL32(100,
        220,
        220,
        255), // component_secondary - Bright cyan
    IM_COL32(80,
        200,
        120,
        255), // component_success - Green
    IM_COL32(255,
        180,
        60,
        255), // component_warning - Orange
    IM_COL32(100,
        180,
        255,
        255) // component_info - Blue
};
