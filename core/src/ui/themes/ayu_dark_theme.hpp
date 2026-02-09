#pragma once

#include "defines.hpp"
#include "imgui.h"
#include "ui/ui_themes.hpp"

// Ayu Dark theme palette
static const UI_Theme_Palette ayu_dark_palette = {
    // Primary colors
    IM_COL32(255, 173, 93, 255),
    IM_COL32(90, 160, 220, 255),
    IM_COL32(115, 175, 255, 255),
    IM_COL32(164, 209, 125, 255),

    // Background colors
    IM_COL32(31, 36, 48, 255),
    IM_COL32(20, 24, 33, 255),
    IM_COL32(17, 21, 29, 255),
    IM_COL32(17, 21, 29, 255),
    IM_COL32(13, 16, 22, 255),
    IM_COL32(41, 47, 63, 255),
    IM_COL32(20, 24, 33, 255),

    // Titlebar gradient colors
    IM_COL32(255, 173, 93, 92),
    IM_COL32(255, 173, 93, 0),

    // Text colors
    IM_COL32(218, 226, 240, 255),
    IM_COL32(240, 245, 255, 255),
    IM_COL32(113, 126, 149, 255),
    IM_COL32(242, 108, 103, 255),

    // UI element colors
    IM_COL32(73, 83, 104, 255),
    IM_COL32(41, 47, 63, 255),
    IM_COL32(255, 173, 93, 255),
    IM_COL32(255, 173, 93, 74),

    // Button colors
    IM_COL32(41, 47, 63, 220),
    IM_COL32(54, 62, 82, 255),
    IM_COL32(66, 75, 98, 255),

    // Tab colors
    IM_COL32(90, 160, 220, 178),
    IM_COL32(90, 160, 220, 126),

    // Resize grip colors
    IM_COL32(204, 212, 226, 64),
    IM_COL32(204, 212, 226, 168),
    IM_COL32(230, 235, 245, 230),

    // Scrollbar colors
    IM_COL32(17, 21, 29, 135),
    IM_COL32(54, 62, 82, 255),
    IM_COL32(73, 83, 104, 255),
    IM_COL32(115, 175, 255, 255),

    // Separator colors
    IM_COL32(255, 173, 93, 150),

    // Docking colors
    IM_COL32(255, 173, 93, 212),

    // Component colors
    IM_COL32(255, 173, 93, 255),
    IM_COL32(90, 160, 220, 255),
    IM_COL32(164, 209, 125, 255),
    IM_COL32(255, 146, 87, 255),
    IM_COL32(115, 175, 255, 255)
};
