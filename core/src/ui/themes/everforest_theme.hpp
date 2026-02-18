#pragma once

#include "defines.hpp"
#include "imgui.h"
#include "ui/ui_themes.hpp"

// Everforest theme palette
static const UI_Theme_Palette everforest_palette = {
    // Primary colors
    IM_COL32(163, 190, 140, 255),
    IM_COL32(131, 165, 152, 255),
    IM_COL32(127, 187, 179, 255),
    IM_COL32(230, 167, 97, 255),

    // Background colors
    IM_COL32(45, 53, 47, 255),
    IM_COL32(35, 43, 37, 255),
    IM_COL32(29, 36, 31, 255),
    IM_COL32(29, 36, 31, 255),
    IM_COL32(24, 29, 25, 255),
    IM_COL32(57, 66, 58, 255),
    IM_COL32(35, 43, 37, 255),

    // Titlebar gradient colors
    IM_COL32(163, 190, 140, 92),
    IM_COL32(163, 190, 140, 0),

    // Text colors
    IM_COL32(224, 212, 185, 255),
    IM_COL32(242, 237, 220, 255),
    IM_COL32(133, 146, 129, 255),
    IM_COL32(230, 126, 128, 255),

    // UI element colors
    IM_COL32(95, 110, 95, 255),
    IM_COL32(57, 66, 58, 255),
    IM_COL32(163, 190, 140, 255),
    IM_COL32(163, 190, 140, 74),

    // Button colors
    IM_COL32(57, 66, 58, 220),
    IM_COL32(70, 81, 72, 255),
    IM_COL32(84, 95, 86, 255),

    // Tab colors
    IM_COL32(131, 165, 152, 176),
    IM_COL32(131, 165, 152, 126),

    // Resize grip colors
    IM_COL32(211, 198, 170, 64),
    IM_COL32(211, 198, 170, 165),
    IM_COL32(230, 224, 204, 228),

    // Scrollbar colors
    IM_COL32(29, 36, 31, 135),
    IM_COL32(70, 81, 72, 255),
    IM_COL32(95, 110, 95, 255),
    IM_COL32(131, 165, 152, 255),

    // Separator colors
    IM_COL32(163, 190, 140, 150),

    // Docking colors
    IM_COL32(163, 190, 140, 210),

    // Component colors
    IM_COL32(163, 190, 140, 255),
    IM_COL32(131, 165, 152, 255),
    IM_COL32(163, 190, 140, 255),
    IM_COL32(230, 167, 97, 255),
    IM_COL32(127, 187, 179, 255)
};
