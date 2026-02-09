#pragma once

#include "defines.hpp"
#include "utils/string.hpp"

// Available UI themes
enum class UI_Theme : u32
{
    DARK = 0,   // Current Walnut-inspired dark theme
    LIGHT,      // Light theme
    CATPPUCCIN, // Catppuccin Mocha color scheme
    TOKYO,      // Tokyo Night color scheme
    DRACULA,    // Dracula color scheme
    NORD,       // Nord color scheme
    GRUVBOX,    // Gruvbox Dark color scheme
    ROSE_PINE,  // Rose Pine color scheme
    KANAGAWA,   // Kanagawa color scheme
    EVERFOREST, // Everforest color scheme
    AYU_DARK,   // Ayu Dark color scheme
    ONE_DARK,   // One Dark color scheme

    MAX_COUNT // Number of available themes
};

// Theme color palette structure
struct UI_Theme_Palette
{
    // Primary colors
    u32 accent;
    u32 highlight;
    u32 nice_blue;
    u32 compliment;

    // Background colors
    u32 background;
    u32 background_dark;
    u32 titlebar;
    u32 window_bg;
    u32 property_field;
    u32 background_popup;
    u32 clear_color; // Window clear/background color

    // Titlebar gradient colors
    u32 titlebar_gradient_start;
    u32 titlebar_gradient_end;

    // Text colors
    u32 text;
    u32 text_brighter;
    u32 text_darker;
    u32 text_error;

    // UI element colors
    u32 muted;
    u32 group_header;
    u32 selection;
    u32 selection_muted;

    // Button colors
    u32 button;
    u32 button_hovered;
    u32 button_active;

    // Tab colors
    u32 tab_hovered;
    u32 tab_active;

    // Resize grip colors
    u32 resize_grip;
    u32 resize_grip_hovered;
    u32 resize_grip_active;

    // Scrollbar colors
    u32 scrollbar_bg;
    u32 scrollbar_grab;
    u32 scrollbar_grab_hovered;
    u32 scrollbar_grab_active;

    // Separator colors
    u32 separator_hovered;

    // Docking colors
    u32 docking_preview;

    // Component colors for custom wrappers
    u32 component_primary;
    u32 component_secondary;
    u32 component_success;
    u32 component_warning;
    u32 component_info;
};

struct UI_Theme_Metadata
{
    String description;
    String keywords;
};

// Apply a theme to ImGui style
void ui_themes_apply(UI_Theme theme, struct ImGuiStyle &style);

// Apply a palette to ImGui style
void ui_themes_apply_palette(const UI_Theme_Palette &palette,
                             ImGuiStyle             &style);

// Copy the palette for a theme into an output buffer
void ui_themes_copy_palette(UI_Theme theme, UI_Theme_Palette *out_palette);

// Get theme display name
String ui_themes_get_name(UI_Theme theme);

// Get theme metadata used by searchable UI components.
UI_Theme_Metadata ui_themes_get_metadata(UI_Theme theme);

// Get clear color from theme palette as ImVec4. Used for viewport background
// coloring
struct ImVec4 ui_themes_get_clear_color(UI_Theme theme);
