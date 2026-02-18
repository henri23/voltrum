#pragma once

#include "defines.hpp"
#include "imgui.h"
#include "ui/ui_themes.hpp"

// One Dark theme palette
static const UI_Theme_Palette one_dark_palette = {
    // Primary colors
    IM_COL32(97, 175, 239, 255),
    IM_COL32(198, 120, 221, 255),
    IM_COL32(86, 182, 194, 255),
    IM_COL32(152, 195, 121, 255),

    // Background colors
    IM_COL32(44, 49, 60, 255),
    IM_COL32(33, 37, 43, 255),
    IM_COL32(24, 27, 33, 255),
    IM_COL32(24, 27, 33, 255),
    IM_COL32(20, 23, 28, 255),
    IM_COL32(55, 61, 74, 255),
    IM_COL32(33, 37, 43, 255),

    // Titlebar gradient colors
    IM_COL32(97, 175, 239, 92),
    IM_COL32(97, 175, 239, 0),

    // Text colors
    IM_COL32(188, 196, 212, 255),
    IM_COL32(236, 239, 244, 255),
    IM_COL32(92, 99, 112, 255),
    IM_COL32(224, 108, 117, 255),

    // UI element colors
    IM_COL32(73, 80, 95, 255),
    IM_COL32(55, 61, 74, 255),
    IM_COL32(97, 175, 239, 255),
    IM_COL32(97, 175, 239, 72),

    // Button colors
    IM_COL32(55, 61, 74, 220),
    IM_COL32(68, 75, 90, 255),
    IM_COL32(83, 91, 109, 255),

    // Tab colors
    IM_COL32(97, 175, 239, 180),
    IM_COL32(97, 175, 239, 128),

    // Resize grip colors
    IM_COL32(171, 178, 191, 64),
    IM_COL32(171, 178, 191, 165),
    IM_COL32(220, 223, 228, 228),

    // Scrollbar colors
    IM_COL32(24, 27, 33, 135),
    IM_COL32(68, 75, 90, 255),
    IM_COL32(83, 91, 109, 255),
    IM_COL32(97, 175, 239, 255),

    // Separator colors
    IM_COL32(97, 175, 239, 150),

    // Docking colors
    IM_COL32(97, 175, 239, 212),

    // Component colors
    IM_COL32(97, 175, 239, 255),
    IM_COL32(198, 120, 221, 255),
    IM_COL32(152, 195, 121, 255),
    IM_COL32(229, 192, 123, 255),
    IM_COL32(86, 182, 194, 255)
};
