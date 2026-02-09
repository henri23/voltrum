#include "ui_themes.hpp"
#include "core/asserts.hpp"
#include "imgui.h"

// Include theme palettes
#include "themes/ayu_dark_theme.hpp"
#include "themes/catppuccin_theme.hpp"
#include "themes/dark_theme.hpp"
#include "themes/dracula_theme.hpp"
#include "themes/everforest_theme.hpp"
#include "themes/gruvbox_theme.hpp"
#include "themes/kanagawa_theme.hpp"
#include "themes/light_theme.hpp"
#include "themes/nord_theme.hpp"
#include "themes/one_dark_theme.hpp"
#include "themes/rose_pine_theme.hpp"
#include "themes/tokyo_theme.hpp"
#include "utils/string.hpp"

// Theme palette array
internal_var const UI_Theme_Palette *theme_palettes[] = {&dark_palette,
                                                         &light_palette,
                                                         &catppuccin_palette,
                                                         &tokyo_palette,
                                                         &dracula_palette,
                                                         &nord_palette,
                                                         &gruvbox_palette,
                                                         &rose_pine_palette,
                                                         &kanagawa_palette,
                                                         &everforest_palette,
                                                         &ayu_dark_palette,
                                                         &one_dark_palette};

// Theme names
static String theme_names[] = {STR_LIT("Dark"),
                               STR_LIT("Light"),
                               STR_LIT("Catppuccin"),
                               STR_LIT("Tokyo"),
                               STR_LIT("Dracula"),
                               STR_LIT("Nord"),
                               STR_LIT("Gruvbox"),
                               STR_LIT("Rose Pine"),
                               STR_LIT("Kanagawa"),
                               STR_LIT("Everforest"),
                               STR_LIT("Ayu Dark"),
                               STR_LIT("One Dark")};

static UI_Theme_Metadata theme_metadata[] = {
    {STR_LIT("Default dark palette"), STR_LIT("theme dark default")},
    {STR_LIT("Default light palette"), STR_LIT("theme light default")},
    {STR_LIT("Catppuccin Mocha palette"), STR_LIT("theme catppuccin mocha")},
    {STR_LIT("Tokyo Night palette"), STR_LIT("theme tokyo night")},
    {STR_LIT("Dracula-inspired palette"), STR_LIT("theme dracula vampire")},
    {STR_LIT("Nord arctic palette"), STR_LIT("theme nord arctic")},
    {STR_LIT("Gruvbox dark palette"), STR_LIT("theme gruvbox retro")},
    {STR_LIT("Rose Pine palette"), STR_LIT("theme rose pine")},
    {STR_LIT("Kanagawa palette"), STR_LIT("theme kanagawa")},
    {STR_LIT("Everforest palette"), STR_LIT("theme everforest forest")},
    {STR_LIT("Ayu Dark palette"), STR_LIT("theme ayu dark")},
    {STR_LIT("One Dark palette"), STR_LIT("theme one dark")},
};

STATIC_ASSERT(sizeof(theme_palettes) / sizeof(theme_palettes[0]) ==
                  (u32)UI_Theme::MAX_COUNT,
              "Theme palette array size must match UI_Theme enum count");

STATIC_ASSERT(sizeof(theme_names) / sizeof(theme_names[0]) ==
                  (u32)UI_Theme::MAX_COUNT,
              "Theme names array size must match UI_Theme enum count");

STATIC_ASSERT(sizeof(theme_metadata) / sizeof(theme_metadata[0]) ==
                  (u32)UI_Theme::MAX_COUNT,
              "Theme metadata array size must match UI_Theme enum count");

void ui_themes_apply_palette(const UI_Theme_Palette &palette,
                             ImGuiStyle             &style);

INTERNAL_FUNC const UI_Theme_Palette &
palette_for_theme(UI_Theme theme)
{
    RUNTIME_ASSERT_MSG((u32)theme < (u32)UI_Theme::MAX_COUNT,
                       "Requested theme is invalid");

    return *theme_palettes[(u32)(theme)];
}

void
ui_themes_apply(UI_Theme theme, ImGuiStyle &style)
{
    UI_Theme_Palette palette = {};
    ui_themes_copy_palette(theme, &palette);
    ui_themes_apply_palette(palette, style);
}

void
ui_themes_apply_palette(const UI_Theme_Palette &palette, ImGuiStyle &style)
{
    ImVec4 *colors = style.Colors;

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

    // Tab colors - theme-specific
    colors[ImGuiCol_Tab] = ImGui::ColorConvertU32ToFloat4(palette.background);
    colors[ImGuiCol_TabHovered] =
        ImGui::ColorConvertU32ToFloat4(palette.tab_hovered);
    colors[ImGuiCol_TabActive] =
        ImGui::ColorConvertU32ToFloat4(palette.tab_active);
    colors[ImGuiCol_TabUnfocused] =
        ImGui::ColorConvertU32ToFloat4(palette.background);
    colors[ImGuiCol_TabUnfocusedActive] =
        ImGui::ColorConvertU32ToFloat4(palette.tab_active);

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

    colors[ImGuiCol_Text]      = ImGui::ColorConvertU32ToFloat4(palette.text);
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
    style.FrameBorderSize = 1.0f;
    style.IndentSpacing   = 11.0f;

    // Global corner rounding
    f32 rounding            = 9.0f;
    style.WindowRounding    = rounding; // Windows
    style.ChildRounding     = rounding; // Child windows
    style.FrameRounding     = rounding; // Frames (buttons, inputs, etc.)
    style.PopupRounding     = rounding; // Popups and tooltips
    style.ScrollbarRounding = rounding; // Scrollbars
    style.GrabRounding      = rounding; // Grab handles (sliders)
    style.TabRounding       = rounding; // Tabs

    // Menu spacing
    style.ItemSpacing =
        ImVec2(12.0f,
               6.0f); // Horizontal between menu items, vertical in dropdowns
    style.FramePadding =
        ImVec2(8.0f, 5.0f); // Padding inside frames/window titles
    style.WindowPadding = ImVec2(10.0f, 10.0f); // Padding inside popup menus
}

String
ui_themes_get_name(UI_Theme theme)
{
    RUNTIME_ASSERT_MSG((u32)theme < (u32)UI_Theme::MAX_COUNT,
                       "Requested theme is invalid");

    return theme_names[(int)theme];
}

UI_Theme_Metadata
ui_themes_get_metadata(UI_Theme theme)
{
    RUNTIME_ASSERT_MSG((u32)theme < (u32)UI_Theme::MAX_COUNT,
                       "Requested theme is invalid");

    return theme_metadata[(int)theme];
}

void
ui_themes_copy_palette(UI_Theme theme, UI_Theme_Palette *out_palette)
{
    if (!out_palette)
    {
        return;
    }

    *out_palette = palette_for_theme(theme);
}

ImVec4
ui_themes_get_clear_color(UI_Theme theme)
{
    const UI_Theme_Palette &palette = palette_for_theme(theme);
    return ImGui::ColorConvertU32ToFloat4(palette.clear_color);
}
