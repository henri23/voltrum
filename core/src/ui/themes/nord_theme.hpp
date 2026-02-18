#pragma once

#include "defines.hpp"
#include "imgui.h"
#include "ui/ui_themes.hpp"

// Nord theme palette
static const UI_Theme_Palette nord_palette = {
    // Primary colors
    IM_COL32(136, 192, 208, 255),
    IM_COL32(129, 161, 193, 255),
    IM_COL32(94, 129, 172, 255),
    IM_COL32(163, 190, 140, 255),

    // Background colors
    IM_COL32(59, 66, 82, 255),
    IM_COL32(46, 52, 64, 255),
    IM_COL32(40, 44, 52, 255),
    IM_COL32(40, 44, 52, 255),
    IM_COL32(36, 40, 49, 255),
    IM_COL32(67, 76, 94, 255),
    IM_COL32(46, 52, 64, 255),

    // Titlebar gradient colors
    IM_COL32(136, 192, 208, 90),
    IM_COL32(136, 192, 208, 0),

    // Text colors
    IM_COL32(244, 247, 252, 255),
    IM_COL32(252, 253, 255, 255),
    IM_COL32(143, 188, 187, 255),
    IM_COL32(191, 97, 106, 255),

    // UI element colors
    IM_COL32(76, 86, 106, 255),
    IM_COL32(67, 76, 94, 255),
    IM_COL32(136, 192, 208, 255),
    IM_COL32(136, 192, 208, 70),

    // Button colors
    IM_COL32(67, 76, 94, 220),
    IM_COL32(76, 86, 106, 255),
    IM_COL32(94, 108, 132, 255),

    // Tab colors
    IM_COL32(129, 161, 193, 180),
    IM_COL32(129, 161, 193, 130),

    // Resize grip colors
    IM_COL32(229, 233, 240, 64),
    IM_COL32(229, 233, 240, 170),
    IM_COL32(236, 239, 244, 232),

    // Scrollbar colors
    IM_COL32(40, 44, 52, 135),
    IM_COL32(76, 86, 106, 255),
    IM_COL32(94, 108, 132, 255),
    IM_COL32(129, 161, 193, 255),

    // Separator colors
    IM_COL32(129, 161, 193, 150),

    // Docking colors
    IM_COL32(136, 192, 208, 215),

    // Component colors
    IM_COL32(136, 192, 208, 255),
    IM_COL32(129, 161, 193, 255),
    IM_COL32(163, 190, 140, 255),
    IM_COL32(235, 203, 139, 255),
    IM_COL32(94, 129, 172, 255)
};
