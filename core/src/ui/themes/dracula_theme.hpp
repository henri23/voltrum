#pragma once

#include "defines.hpp"
#include "imgui.h"
#include "ui/ui_themes.hpp"

// Dracula theme palette
static const UI_Theme_Palette dracula_palette = {
    // Primary colors
    IM_COL32(255, 121, 198, 255),
    IM_COL32(189, 147, 249, 255),
    IM_COL32(139, 233, 253, 255),
    IM_COL32(80, 250, 123, 255),

    // Background colors
    IM_COL32(52, 55, 70, 255),
    IM_COL32(40, 42, 54, 255),
    IM_COL32(32, 34, 44, 255),
    IM_COL32(32, 34, 44, 255),
    IM_COL32(27, 28, 36, 255),
    IM_COL32(58, 61, 79, 255),
    IM_COL32(30, 31, 40, 255),

    // Titlebar gradient colors
    IM_COL32(255, 121, 198, 95),
    IM_COL32(255, 121, 198, 0),

    // Text colors
    IM_COL32(252, 252, 248, 255),
    IM_COL32(255, 255, 255, 255),
    IM_COL32(147, 153, 178, 255),
    IM_COL32(255, 85, 85, 255),

    // UI element colors
    IM_COL32(98, 114, 164, 255),
    IM_COL32(68, 71, 90, 255),
    IM_COL32(189, 147, 249, 255),
    IM_COL32(189, 147, 249, 72),

    // Button colors
    IM_COL32(68, 71, 90, 220),
    IM_COL32(80, 84, 105, 255),
    IM_COL32(94, 99, 124, 255),

    // Tab colors
    IM_COL32(189, 147, 249, 180),
    IM_COL32(189, 147, 249, 132),

    // Resize grip colors
    IM_COL32(248, 248, 242, 64),
    IM_COL32(248, 248, 242, 166),
    IM_COL32(255, 255, 255, 230),

    // Scrollbar colors
    IM_COL32(32, 34, 44, 135),
    IM_COL32(80, 84, 105, 255),
    IM_COL32(98, 114, 164, 255),
    IM_COL32(139, 233, 253, 255),

    // Separator colors
    IM_COL32(189, 147, 249, 150),

    // Docking colors
    IM_COL32(255, 121, 198, 220),

    // Component colors
    IM_COL32(255, 121, 198, 255),
    IM_COL32(189, 147, 249, 255),
    IM_COL32(80, 250, 123, 255),
    IM_COL32(255, 184, 108, 255),
    IM_COL32(139, 233, 253, 255)
};
