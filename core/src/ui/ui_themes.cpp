#include "ui_themes.hpp"
#include "imgui.h"

// Include theme palettes
#include "themes/catppuccin_mocha_theme.hpp"
#include "themes/dark_theme.hpp"
#include "themes/light_theme.hpp"

// Theme palette array
internal_var const UI_Theme_Palette* theme_palettes[] = {&dark_palette,
    &light_palette,
    &catppuccin_mocha_palette};

// Theme names
static const char* theme_names[] = {"Dark", "Light", "Catppuccin Mocha"};

STATIC_ASSERT(sizeof(theme_palettes) / sizeof(theme_palettes[0]) ==
                  (int)UI_Theme::COUNT,
    "Theme palette array size must match UI_Theme enum count");

STATIC_ASSERT(sizeof(theme_names) / sizeof(theme_names[0]) ==
                  (int)UI_Theme::COUNT,
    "Theme names array size must match UI_Theme enum count");

void ui_themes_apply(UI_Theme theme, ImGuiStyle& style) {
    if ((int)theme >= (int)UI_Theme::COUNT) {
        theme = UI_Theme::DARK; // Fallback to dark theme
    }

    const UI_Theme_Palette& palette = *theme_palettes[(int)theme];
    ImVec4* colors = style.Colors;

    // Apply theme colors to ImGui style - using theme-specific colors
    colors[ImGuiCol_Header] =
        ImGui::ColorConvertU32ToFloat4(palette.group_header);
    colors[ImGuiCol_HeaderHovered] =
        ImGui::ColorConvertU32ToFloat4(palette.group_header);
    colors[ImGuiCol_HeaderActive] =
        ImGui::ColorConvertU32ToFloat4(palette.group_header);

    // Button colors - now theme-specific
    colors[ImGuiCol_Button] = ImGui::ColorConvertU32ToFloat4(palette.button);
    colors[ImGuiCol_ButtonHovered] =
        ImGui::ColorConvertU32ToFloat4(palette.button_hovered);
    colors[ImGuiCol_ButtonActive] =
        ImGui::ColorConvertU32ToFloat4(palette.button_active);

    colors[ImGuiCol_FrameBg] =
        ImGui::ColorConvertU32ToFloat4(palette.property_field);
    colors[ImGuiCol_FrameBgHovered] =
        ImGui::ColorConvertU32ToFloat4(palette.property_field);
    colors[ImGuiCol_FrameBgActive] =
        ImGui::ColorConvertU32ToFloat4(palette.property_field);

    // Tab colors - now theme-specific
    colors[ImGuiCol_Tab] = ImGui::ColorConvertU32ToFloat4(palette.titlebar);
    colors[ImGuiCol_TabHovered] =
        ImGui::ColorConvertU32ToFloat4(palette.tab_hovered);
    colors[ImGuiCol_TabActive] =
        ImGui::ColorConvertU32ToFloat4(palette.tab_active);
    colors[ImGuiCol_TabUnfocused] =
        ImGui::ColorConvertU32ToFloat4(palette.titlebar);
    colors[ImGuiCol_TabUnfocusedActive] = colors[ImGuiCol_TabHovered];

    colors[ImGuiCol_TitleBg] = ImGui::ColorConvertU32ToFloat4(palette.titlebar);
    colors[ImGuiCol_TitleBgActive] =
        ImGui::ColorConvertU32ToFloat4(palette.titlebar);
    colors[ImGuiCol_TitleBgCollapsed] =
        ImGui::ColorConvertU32ToFloat4(palette.background_dark);

    // Resize grip colors - now theme-specific
    colors[ImGuiCol_ResizeGrip] =
        ImGui::ColorConvertU32ToFloat4(palette.resize_grip);
    colors[ImGuiCol_ResizeGripHovered] =
        ImGui::ColorConvertU32ToFloat4(palette.resize_grip_hovered);
    colors[ImGuiCol_ResizeGripActive] =
        ImGui::ColorConvertU32ToFloat4(palette.resize_grip_active);

    // Scrollbar colors - now theme-specific
    colors[ImGuiCol_ScrollbarBg] =
        ImGui::ColorConvertU32ToFloat4(palette.scrollbar_bg);
    colors[ImGuiCol_ScrollbarGrab] =
        ImGui::ColorConvertU32ToFloat4(palette.scrollbar_grab);
    colors[ImGuiCol_ScrollbarGrabHovered] =
        ImGui::ColorConvertU32ToFloat4(palette.scrollbar_grab_hovered);
    colors[ImGuiCol_ScrollbarGrabActive] =
        ImGui::ColorConvertU32ToFloat4(palette.scrollbar_grab_active);

    colors[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(palette.text);
    colors[ImGuiCol_CheckMark] = ImGui::ColorConvertU32ToFloat4(palette.text);
    colors[ImGuiCol_Separator] =
        ImGui::ColorConvertU32ToFloat4(palette.background_dark);
    colors[ImGuiCol_SeparatorActive] =
        ImGui::ColorConvertU32ToFloat4(palette.highlight);

    // Separator hover - now theme-specific
    colors[ImGuiCol_SeparatorHovered] =
        ImGui::ColorConvertU32ToFloat4(palette.separator_hovered);

    colors[ImGuiCol_WindowBg] =
        ImGui::ColorConvertU32ToFloat4(palette.window_bg);
    colors[ImGuiCol_ChildBg] =
        ImGui::ColorConvertU32ToFloat4(palette.background);
    colors[ImGuiCol_PopupBg] =
        ImGui::ColorConvertU32ToFloat4(palette.background_popup);
    colors[ImGuiCol_Border] =
        ImGui::ColorConvertU32ToFloat4(palette.background_dark);
    colors[ImGuiCol_TableHeaderBg] =
        ImGui::ColorConvertU32ToFloat4(palette.group_header);
    colors[ImGuiCol_TableBorderLight] =
        ImGui::ColorConvertU32ToFloat4(palette.background_dark);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    // Docking colors - now theme-specific
    colors[ImGuiCol_DockingPreview] =
        ImGui::ColorConvertU32ToFloat4(palette.docking_preview);

    // Apply style tweaks
    style.FrameRounding = 2.5f;
    style.FrameBorderSize = 1.0f;
    style.IndentSpacing = 11.0f;
}

const char* ui_themes_get_name(UI_Theme theme) {
    if ((int)theme >= (int)UI_Theme::COUNT) {
        return "Unknown";
    }
    return theme_names[(int)theme];
}

const UI_Theme_Palette& ui_themes_get_palette(UI_Theme theme) {
    if ((int)theme >= (int)UI_Theme::COUNT) {
        theme = UI_Theme::DARK; // Fallback to dark theme
    }
    return *theme_palettes[(int)theme];
}

ImVec4 ui_themes_get_clear_color(UI_Theme theme) {
    if ((int)theme >= (int)UI_Theme::COUNT) {
        theme = UI_Theme::DARK; // Fallback to dark theme
    }
    const UI_Theme_Palette& palette = *theme_palettes[(int)theme];
    return ImGui::ColorConvertU32ToFloat4(palette.clear_color);
}
