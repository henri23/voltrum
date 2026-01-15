#pragma once

#include "defines.hpp"
#include "imgui.h"
#include "ui/ui_themes.hpp"

// Catppuccin Mocha theme palette - C++17 compatible
static const UI_Theme_Palette catppuccin_palette = {
    // Primary colors (Catppuccin Mocha)
    IM_COL32(245,
        194,
        231,
        255), // accent (Pink)
    IM_COL32(137,
        180,
        250,
        255), // highlight (Blue)
    IM_COL32(116,
        199,
        236,
        255), // nice_blue (Sky)
    IM_COL32(148,
        226,
        213,
        255), // compliment (Teal)

    // Background colors (Catppuccin Mocha)
    IM_COL32(40,
        42,
        54,
        255), // background (Surface0)
    IM_COL32(24,
        24,
        37,
        255), // background_dark (Base)
    IM_COL32(17,
        17,
        27,
        255), // titlebar (Mantle)
    IM_COL32(17,
        17,
        27,
        255), // window_bg (Mantle)
    IM_COL32(11,
        11,
        18,
        255), // property_field (Crust)
    IM_COL32(54,
        56,
        72,
        255), // background_popup (Surface1)
    IM_COL32(24,
        24,
        37,
        255), // clear_color (Base)

    // Titlebar gradient colors (Catppuccin Mocha)
    IM_COL32(203,
        166,
        247,
        60), // titlebar_gradient_start - Mauve with transparency
    IM_COL32(203,
        166,
        247,
        0), // titlebar_gradient_end - Mauve fully transparent

    // Text colors (Catppuccin Mocha)
    IM_COL32(220,
        224,
        252,
        255), // text (Text)
    IM_COL32(186,
        194,
        222,
        255), // text_brighter (Subtext1)
    IM_COL32(147,
        153,
        178,
        255), // text_darker (Subtext0)
    IM_COL32(243,
        139,
        168,
        255), // text_error (Red)

    // UI element colors (Catppuccin Mocha)
    IM_COL32(88,
        91,
        112,
        255), // muted (Surface2)
    IM_COL32(69,
        71,
        90,
        255), // group_header (Surface1)
    IM_COL32(203,
        166,
        247,
        255), // selection (Mauve)
    IM_COL32(203,
        166,
        247,
        60), // selection_muted (Mauve with alpha)

    // Button colors (Catppuccin Mocha)
    IM_COL32(69,
        71,
        90,
        200), // button (Surface1 with alpha)
    IM_COL32(88,
        91,
        112,
        255), // button_hovered (Surface2)
    IM_COL32(49,
        50,
        68,
        255), // button_active (Surface0)

    // Tab colors (Catppuccin Mocha)
    IM_COL32(203,
        166,
        247,
        77), // tab_hovered (Mauve 30% alpha)
    IM_COL32(203,
        166,
        247,
        153), // tab_active (Mauve 60% alpha)

    // Resize grip colors (Catppuccin Mocha)
    IM_COL32(166,
        173,
        200,
        64), // resize_grip (Subtext1 25% alpha)
    IM_COL32(166,
        173,
        200,
        171), // resize_grip_hovered (Subtext1 67% alpha)
    IM_COL32(205,
        214,
        244,
        242), // resize_grip_active (Text 95% alpha)

    // Scrollbar colors (Catppuccin Mocha)
    IM_COL32(17,
        17,
        27,
        135), // scrollbar_bg (Crust 53% alpha)
    IM_COL32(88,
        91,
        112,
        255), // scrollbar_grab (Surface2)
    IM_COL32(108,
        112,
        134,
        255), // scrollbar_grab_hovered (Overlay0)
    IM_COL32(127,
        132,
        156,
        255), // scrollbar_grab_active (Subtext0)

    // Separator colors (Catppuccin Mocha)
    IM_COL32(137,
        180,
        250,
        150), // separator_hovered (Blue with alpha)

    // Docking colors (Catppuccin Mocha)
    IM_COL32(245,
        194,
        231,
        255), // docking_preview (Pink)

    // Component colors for custom wrappers (Catppuccin Mocha)
    IM_COL32(245,
        194,
        231,
        255), // component_primary - Pink
    IM_COL32(203,
        166,
        247,
        255), // component_secondary - Mauve
    IM_COL32(166,
        227,
        161,
        255), // component_success - Green
    IM_COL32(250,
        179,
        135,
        255), // component_warning - Peach
    IM_COL32(137,
        180,
        250,
        255) // component_info - Blue
};
