#pragma once

#include "defines.hpp"
#include "imgui.h"
#include "ui/ui_themes.hpp"

// Kanagawa theme palette
static const UI_Theme_Palette kanagawa_palette = {
    // Primary colors
    IM_COL32(125, 159, 175, 255),
    IM_COL32(126, 156, 216, 255),
    IM_COL32(98, 123, 156, 255),
    IM_COL32(152, 187, 108, 255),

    // Background colors
    IM_COL32(45, 52, 63, 255),
    IM_COL32(31, 36, 46, 255),
    IM_COL32(24, 28, 36, 255),
    IM_COL32(24, 28, 36, 255),
    IM_COL32(19, 22, 29, 255),
    IM_COL32(54, 63, 79, 255),
    IM_COL32(31, 36, 46, 255),

    // Titlebar gradient colors
    IM_COL32(125, 159, 175, 88),
    IM_COL32(125, 159, 175, 0),

    // Text colors
    IM_COL32(232, 227, 198, 255),
    IM_COL32(241, 241, 234, 255),
    IM_COL32(114, 123, 137, 255),
    IM_COL32(196, 82, 75, 255),

    // UI element colors
    IM_COL32(87, 97, 110, 255),
    IM_COL32(54, 63, 79, 255),
    IM_COL32(125, 159, 175, 255),
    IM_COL32(125, 159, 175, 72),

    // Button colors
    IM_COL32(54, 63, 79, 220),
    IM_COL32(66, 77, 96, 255),
    IM_COL32(79, 90, 112, 255),

    // Tab colors
    IM_COL32(126, 156, 216, 178),
    IM_COL32(126, 156, 216, 126),

    // Resize grip colors
    IM_COL32(220, 215, 186, 64),
    IM_COL32(220, 215, 186, 168),
    IM_COL32(235, 232, 210, 230),

    // Scrollbar colors
    IM_COL32(24, 28, 36, 135),
    IM_COL32(66, 77, 96, 255),
    IM_COL32(87, 97, 110, 255),
    IM_COL32(126, 156, 216, 255),

    // Separator colors
    IM_COL32(125, 159, 175, 150),

    // Docking colors
    IM_COL32(125, 159, 175, 212),

    // Component colors
    IM_COL32(125, 159, 175, 255),
    IM_COL32(126, 156, 216, 255),
    IM_COL32(152, 187, 108, 255),
    IM_COL32(255, 160, 102, 255),
    IM_COL32(98, 123, 156, 255)
};
