#pragma once

#include "defines.hpp"
#include "imgui.h"
#include "ui/ui_themes.hpp"

// Rose Pine theme palette
static const UI_Theme_Palette rose_pine_palette = {
    // Primary colors
    IM_COL32(235, 111, 146, 255),
    IM_COL32(196, 167, 231, 255),
    IM_COL32(156, 207, 216, 255),
    IM_COL32(49, 116, 143, 255),

    // Background colors
    IM_COL32(35, 33, 54, 255),
    IM_COL32(25, 23, 36, 255),
    IM_COL32(19, 17, 28, 255),
    IM_COL32(19, 17, 28, 255),
    IM_COL32(15, 13, 22, 255),
    IM_COL32(49, 46, 73, 255),
    IM_COL32(25, 23, 36, 255),

    // Titlebar gradient colors
    IM_COL32(196, 167, 231, 92),
    IM_COL32(196, 167, 231, 0),

    // Text colors
    IM_COL32(236, 234, 255, 255),
    IM_COL32(246, 240, 255, 255),
    IM_COL32(144, 140, 170, 255),
    IM_COL32(235, 111, 146, 255),

    // UI element colors
    IM_COL32(110, 106, 134, 255),
    IM_COL32(49, 46, 73, 255),
    IM_COL32(196, 167, 231, 255),
    IM_COL32(196, 167, 231, 74),

    // Button colors
    IM_COL32(49, 46, 73, 220),
    IM_COL32(68, 64, 96, 255),
    IM_COL32(84, 79, 120, 255),

    // Tab colors
    IM_COL32(196, 167, 231, 180),
    IM_COL32(196, 167, 231, 130),

    // Resize grip colors
    IM_COL32(224, 222, 244, 64),
    IM_COL32(224, 222, 244, 168),
    IM_COL32(246, 240, 255, 228),

    // Scrollbar colors
    IM_COL32(19, 17, 28, 135),
    IM_COL32(68, 64, 96, 255),
    IM_COL32(84, 79, 120, 255),
    IM_COL32(156, 207, 216, 255),

    // Separator colors
    IM_COL32(196, 167, 231, 152),

    // Docking colors
    IM_COL32(235, 111, 146, 210),

    // Component colors
    IM_COL32(235, 111, 146, 255),
    IM_COL32(196, 167, 231, 255),
    IM_COL32(49, 116, 143, 255),
    IM_COL32(246, 193, 119, 255),
    IM_COL32(156, 207, 216, 255)
};
