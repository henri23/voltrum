#pragma once

#include "defines.hpp"
#include "ui/ui_themes.hpp"
#include "imgui.h"

// Tokyo Night theme palette - C++17 compatible
static const UI_Theme_Palette tokyo_palette = {
    // Primary colors (Tokyo Night)
    IM_COL32(122,
        162,
        247,
        255), // accent (Blue)
    IM_COL32(122,
        162,
        247,
        255), // highlight (Blue)
    IM_COL32(125,
        207,
        255,
        255), // nice_blue (Cyan)
    IM_COL32(158,
        206,
        106,
        255), // compliment (Green)

    // Background colors (Tokyo Night)
    IM_COL32(36,
        40,
        59,
        255), // background (bg_highlight)
    IM_COL32(26,
        27,
        38,
        255), // background_dark (bg)
    IM_COL32(22,
        22,
        30,
        255), // titlebar (bg_dark)
    IM_COL32(22,
        22,
        30,
        255), // window_bg (bg_dark)
    IM_COL32(15,
        15,
        20,
        255), // property_field (darker)
    IM_COL32(41,
        46,
        66,
        255), // background_popup (bg_highlight)
    IM_COL32(26,
        27,
        38,
        255), // clear_color (bg)

    // Titlebar gradient colors (Tokyo Night)
    IM_COL32(122,
        162,
        247,
        100), // titlebar_gradient_start - Blue with more opacity
    IM_COL32(122,
        162,
        247,
        0), // titlebar_gradient_end - Blue fully transparent

    // Text colors (Tokyo Night)
    IM_COL32(192,
        202,
        245,
        255), // text (fg)
    IM_COL32(169,
        177,
        214,
        255), // text_brighter (fg_dark)
    IM_COL32(86,
        95,
        137,
        255), // text_darker (comment)
    IM_COL32(247,
        118,
        142,
        255), // text_error (red)

    // UI element colors (Tokyo Night)
    IM_COL32(86,
        95,
        137,
        255), // muted (comment)
    IM_COL32(41,
        46,
        66,
        255), // group_header (bg_highlight)
    IM_COL32(122,
        162,
        247,
        255), // selection (blue)
    IM_COL32(122,
        162,
        247,
        60), // selection_muted (blue with alpha)

    // Button colors (Tokyo Night)
    IM_COL32(41,
        46,
        66,
        200), // button (bg_highlight with alpha)
    IM_COL32(40,
        52,
        87,
        255), // button_hovered (tokyo night selection blue #283457)
    IM_COL32(50,
        65,
        110,
        255), // button_active (brighter blue when pressed)

    // Tab colors (Tokyo Night)
    IM_COL32(40,
        52,
        87,
        255), // tab_hovered (tokyo night selection blue)
    IM_COL32(50,
        65,
        110,
        255), // tab_active (brighter blue)

    // Resize grip colors (Tokyo Night)
    IM_COL32(169,
        177,
        214,
        64), // resize_grip (fg_dark 25% alpha)
    IM_COL32(169,
        177,
        214,
        171), // resize_grip_hovered (fg_dark 67% alpha)
    IM_COL32(192,
        202,
        245,
        242), // resize_grip_active (fg 95% alpha)

    // Scrollbar colors (Tokyo Night)
    IM_COL32(22,
        22,
        30,
        135), // scrollbar_bg (bg_dark 53% alpha)
    IM_COL32(59,
        66,
        97,
        255), // scrollbar_grab (terminal_black)
    IM_COL32(86,
        95,
        137,
        255), // scrollbar_grab_hovered (comment)
    IM_COL32(122,
        162,
        247,
        255), // scrollbar_grab_active (blue)

    // Separator colors (Tokyo Night)
    IM_COL32(122,
        162,
        247,
        150), // separator_hovered (blue with alpha)

    // Docking colors (Tokyo Night)
    IM_COL32(122,
        162,
        247,
        255), // docking_preview (blue)

    // Component colors for custom wrappers (Tokyo Night)
    IM_COL32(122,
        162,
        247,
        255), // component_primary - Blue
    IM_COL32(122,
        162,
        247,
        255), // component_secondary - Blue
    IM_COL32(158,
        206,
        106,
        255), // component_success - Green
    IM_COL32(255,
        158,
        100,
        255), // component_warning - Orange
    IM_COL32(125,
        207,
        255,
        255) // component_info - Cyan
};
