#pragma once

#include "defines.hpp"
#include "imgui.h"
#include "ui/ui_themes.hpp"

// Light theme palette - C++17 compatible
static const UI_Theme_Palette light_palette = {
    // Primary colors
    IM_COL32(40,
        80,
        180,
        255), // accent - Deep blue
    IM_COL32(50,
        100,
        200,
        255), // highlight - Bright blue
    IM_COL32(70,
        130,
        210,
        255), // nice_blue - Sky blue
    IM_COL32(30,
        70,
        150,
        255), // compliment - Navy blue

    // Background colors
    IM_COL32(240,
        240,
        240,
        255), // background - Light gray
    IM_COL32(250,
        250,
        250,
        255), // background_dark - Very light gray
    IM_COL32(245,
        245,
        245,
        255), // titlebar - Light gray
    IM_COL32(245,
        245,
        245,
        255), // window_bg - Light gray
    IM_COL32(255,
        255,
        255,
        255), // property_field - White
    IM_COL32(235,
        235,
        235,
        255), // background_popup - Medium light gray
    IM_COL32(248,
        248,
        248,
        255), // clear_color - Off-white

    // Titlebar gradient colors
    IM_COL32(40,
        80,
        180,
        100), // titlebar_gradient_start - Deep blue with transparency
    IM_COL32(40,
        80,
        180,
        0), // titlebar_gradient_end - Deep blue fully transparent

    // Text colors
    IM_COL32(40,
        40,
        40,
        255), // text - Dark gray
    IM_COL32(20,
        20,
        20,
        255), // text_brighter - Almost black
    IM_COL32(120,
        120,
        120,
        255), // text_darker - Medium gray
    IM_COL32(200,
        40,
        40,
        255), // text_error - Red

    // UI element colors
    IM_COL32(220,
        220,
        220,
        255), // muted - Light gray
    IM_COL32(230,
        230,
        230,
        255), // group_header - Very light gray
    IM_COL32(40,
        80,
        180,
        255), // selection - Deep blue (matches accent)
    IM_COL32(40,
        80,
        180,
        100), // selection_muted - Deep blue with alpha

    // Button colors
    IM_COL32(230,
        230,
        230,
        255), // button - Light gray
    IM_COL32(220,
        220,
        220,
        255), // button_hovered - Slightly darker
    IM_COL32(210,
        210,
        210,
        255), // button_active - Even darker

    // Tab colors
    IM_COL32(40,
        80,
        180,
        120), // tab_hovered - Deep blue with alpha
    IM_COL32(40,
        80,
        180,
        180), // tab_active - Deep blue with more alpha

    // Resize grip colors
    IM_COL32(100,
        100,
        100,
        64), // resize_grip - Dark gray (25% alpha)
    IM_COL32(80,
        80,
        80,
        171), // resize_grip_hovered - Darker gray (67% alpha)
    IM_COL32(60,
        60,
        60,
        242), // resize_grip_active - Even darker (95% alpha)

    // Scrollbar colors
    IM_COL32(250,
        250,
        250,
        135), // scrollbar_bg - Very light gray
    IM_COL32(200,
        200,
        200,
        255), // scrollbar_grab - Medium gray
    IM_COL32(180,
        180,
        180,
        255), // scrollbar_grab_hovered - Darker gray
    IM_COL32(160,
        160,
        160,
        255), // scrollbar_grab_active - Even darker

    // Separator colors
    IM_COL32(40,
        80,
        180,
        180), // separator_hovered - Deep blue

    // Docking colors
    IM_COL32(40,
        80,
        180,
        200), // docking_preview - Deep blue

    // Component colors for custom wrappers
    IM_COL32(40,
        80,
        180,
        255), // component_primary - Deep blue accent
    IM_COL32(50,
        100,
        200,
        255), // component_secondary - Bright blue
    IM_COL32(40,
        160,
        70,
        255), // component_success - Green
    IM_COL32(230,
        140,
        30,
        255), // component_warning - Orange
    IM_COL32(0,
        120,
        215,
        255) // component_info - Blue
};
