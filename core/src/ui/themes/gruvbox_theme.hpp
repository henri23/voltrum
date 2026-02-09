#pragma once

#include "defines.hpp"
#include "imgui.h"
#include "ui/ui_themes.hpp"

// Gruvbox Dark theme palette
static const UI_Theme_Palette gruvbox_palette = {
    // Primary colors
    IM_COL32(250, 189, 47, 255),
    IM_COL32(184, 187, 38, 255),
    IM_COL32(131, 165, 152, 255),
    IM_COL32(251, 73, 52, 255),

    // Background colors
    IM_COL32(60, 56, 54, 255),
    IM_COL32(40, 40, 40, 255),
    IM_COL32(29, 32, 33, 255),
    IM_COL32(29, 32, 33, 255),
    IM_COL32(24, 24, 24, 255),
    IM_COL32(80, 73, 69, 255),
    IM_COL32(40, 40, 40, 255),

    // Titlebar gradient colors
    IM_COL32(250, 189, 47, 88),
    IM_COL32(250, 189, 47, 0),

    // Text colors
    IM_COL32(246, 231, 191, 255),
    IM_COL32(255, 248, 214, 255),
    IM_COL32(146, 131, 116, 255),
    IM_COL32(204, 36, 29, 255),

    // UI element colors
    IM_COL32(124, 111, 100, 255),
    IM_COL32(80, 73, 69, 255),
    IM_COL32(250, 189, 47, 255),
    IM_COL32(250, 189, 47, 72),

    // Button colors
    IM_COL32(80, 73, 69, 220),
    IM_COL32(102, 92, 84, 255),
    IM_COL32(124, 111, 100, 255),

    // Tab colors
    IM_COL32(184, 187, 38, 180),
    IM_COL32(184, 187, 38, 130),

    // Resize grip colors
    IM_COL32(235, 219, 178, 64),
    IM_COL32(235, 219, 178, 168),
    IM_COL32(251, 241, 199, 228),

    // Scrollbar colors
    IM_COL32(29, 32, 33, 135),
    IM_COL32(102, 92, 84, 255),
    IM_COL32(124, 111, 100, 255),
    IM_COL32(250, 189, 47, 255),

    // Separator colors
    IM_COL32(250, 189, 47, 152),

    // Docking colors
    IM_COL32(250, 189, 47, 210),

    // Component colors
    IM_COL32(250, 189, 47, 255),
    IM_COL32(184, 187, 38, 255),
    IM_COL32(184, 187, 38, 255),
    IM_COL32(254, 128, 25, 255),
    IM_COL32(131, 165, 152, 255)
};
