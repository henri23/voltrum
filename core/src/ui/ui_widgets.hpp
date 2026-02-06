#pragma once

#include "defines.hpp"
#include "ui_themes.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

// Custom menu widgets with rounded corners

namespace ui {

    inline ImU32 WithAlpha(ImU32 color, f32 alpha_scale) {
        ImVec4 c = ImGui::ColorConvertU32ToFloat4(color);
        c.w = ImClamp(c.w * alpha_scale, 0.0f, 1.0f);
        return ImGui::ColorConvertFloat4ToU32(c);
    }

    inline ImU32 BlendColor(ImU32 a, ImU32 b, f32 t) {
        ImVec4 ca = ImGui::ColorConvertU32ToFloat4(a);
        ImVec4 cb = ImGui::ColorConvertU32ToFloat4(b);
        ImVec4 c = ImLerp(ca, cb, ImClamp(t, 0.0f, 1.0f));
        return ImGui::ColorConvertFloat4ToU32(c);
    }

    struct GlassStyle {
        ImU32 fill_idle;
        ImU32 fill_hover;
        ImU32 fill_active;
        ImU32 border;
        ImU32 text_idle;
        ImU32 text_hover;
        ImU32 accent;
        f32 rounding;
    };

    inline GlassStyle MakeGlassStyle(
        const UI_Theme_Palette& palette,
        f32 accent_mix = 0.22f) {
        const ImGuiStyle& style = ImGui::GetStyle();

        ImU32 accent_tint = WithAlpha(palette.accent, 0.30f);
        ImU32 base_fill = WithAlpha(palette.background_popup, 0.78f);

        GlassStyle s = {};
        s.fill_idle = base_fill;
        s.fill_hover = BlendColor(base_fill, accent_tint, accent_mix);
        s.fill_active = BlendColor(base_fill, accent_tint, accent_mix + 0.18f);
        s.border = WithAlpha(palette.selection_muted, 0.72f);
        s.text_idle = palette.text;
        s.text_hover = palette.text_brighter;
        s.accent = WithAlpha(palette.accent, 0.45f);
        s.rounding = ImMax(4.0f, style.FrameRounding);
        return s;
    }

    inline void GlassSurface(
        ImDrawList* draw_list,
        const ImVec2& min,
        const ImVec2& max,
        const GlassStyle& style,
        ImU32 fill_override = 0,
        ImU32 border_override = 0,
        bool draw_accent = true) {
        ImU32 fill = (fill_override != 0) ? fill_override : style.fill_idle;
        ImU32 border = (border_override != 0) ? border_override : style.border;
        draw_list->AddRectFilled(min, max, fill, style.rounding);
        draw_list->AddRect(min, max, border, style.rounding);

        if (draw_accent) {
            draw_list->AddLine(
                ImVec2(min.x + 2.0f, min.y + 1.0f),
                ImVec2(max.x - 2.0f, min.y + 1.0f),
                style.accent,
                1.0f);
        }
    }

    inline bool GlassIconButton(
        const char* id,
        const char* icon,
        const char* tooltip,
        ImDrawList* draw_list,
        const ImVec2& min,
        const ImVec2& size,
        const GlassStyle& style,
        bool draw_accent = true) {
        ImGui::SetCursorScreenPos(min);
        bool clicked = ImGui::InvisibleButton(id, size);
        bool hovered = ImGui::IsItemHovered();
        bool active = ImGui::IsItemActive();

        ImU32 fill = style.fill_idle;
        if (hovered)
            fill = style.fill_hover;
        if (active)
            fill = style.fill_active;

        ImVec2 max(min.x + size.x, min.y + size.y);
        GlassSurface(draw_list, min, max, style, fill, 0, draw_accent);

        ImVec2 text_size = ImGui::CalcTextSize(icon);
        // FontAwesome glyph bounds can look top-clipped with strict centering.
        const f32 text_y_bias = 1.0f;
        draw_list->AddText(
            ImVec2(
                min.x + (size.x - text_size.x) * 0.5f,
                min.y + (size.y - text_size.y) * 0.5f + text_y_bias),
            hovered ? style.text_hover : style.text_idle,
            icon);

        if (hovered && tooltip)
            ImGui::SetTooltip("%s", tooltip);

        return clicked;
    }

    // Rounded MenuItem with optional active-state checkmark at the end.
    // If is_active is provided, a checkmark is rendered on the right when
    // *is_active is true, and pressing the item auto-toggles the value.
    inline bool MenuItem(const char *label,
        const char *shortcut = nullptr,
        b8 *is_active = nullptr,
        bool enabled = true) {
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext &g = *GImGui;
        ImGuiStyle &style = g.Style;
        ImVec2 pos = window->DC.CursorPos;
        ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);

        ImVec2 shortcut_size =
            shortcut ? ImGui::CalcTextSize(shortcut) : ImVec2(0, 0);
        float shortcut_w = shortcut_size.x > 0
                               ? shortcut_size.x + style.ItemSpacing.x * 2.0f
                               : 0.0f;

        float check_sz = g.FontSize;
        float check_w = is_active
                            ? check_sz + style.ItemSpacing.x
                            : 0.0f;

        float min_w = window->DC.MenuColumns.TotalWidth + shortcut_w + check_w;
        float avail_w = ImGui::GetContentRegionAvail().x;
        float w = ImMax(
            label_size.x + style.FramePadding.x * 2.0f + shortcut_w + check_w,
            ImMax(min_w, avail_w));

        ImVec2 size(w, label_size.y + style.FramePadding.y * 2.0f);

        ImRect bb(pos, pos + size);
        ImGui::ItemSize(size, style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, ImGui::GetID(label)))
            return false;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb,
            ImGui::GetID(label),
            &hovered,
            &held,
            ImGuiButtonFlags_PressedOnRelease);

        // Render rounded background on hover
        if (hovered && enabled) {
            ImU32 col = ImGui::GetColorU32(
                held ? ImGuiCol_HeaderActive : ImGuiCol_HeaderHovered);
            window->DrawList->AddRectFilled(
                bb.Min, bb.Max, col, style.WindowRounding);
        }

        // Render text
        ImVec2 text_pos = bb.Min + style.FramePadding;
        ImGui::RenderText(text_pos, label);

        // Render shortcut
        if (shortcut) {
            float shortcut_x =
                bb.Max.x - style.FramePadding.x - shortcut_size.x - check_w;
            ImGui::RenderText(
                ImVec2(shortcut_x, text_pos.y), shortcut);
        }

        // Render checkmark at the right end when is_active is provided
        if (is_active && *is_active) {
            ImU32 col = ImGui::GetColorU32(
                enabled ? ImGuiCol_Text : ImGuiCol_TextDisabled);
            float pad = ImMax(1.0f, (float)(int)(check_sz / 6.0f));
            float check_x = bb.Max.x - style.FramePadding.x - check_sz;
            float check_y = bb.Min.y + style.FramePadding.y + pad;
            ImGui::RenderCheckMark(
                window->DrawList,
                ImVec2(check_x, check_y),
                col,
                check_sz - pad * 2.0f);
        }

        // Toggle active state and close popup when activated
        if (pressed && enabled) {
            if (is_active)
                *is_active = !(*is_active);
            ImGui::CloseCurrentPopup();
        }

        return pressed && enabled;
    }

    // Storage for menu hover state
    struct MenuState {
        ImGuiID active_menu_id = 0;
        ImRect active_menu_rect = {};
        bool alpha_pushed = false;
    };

    inline MenuState &GetMenuState() {
        static MenuState state;
        return state;
    }

    // Rounded BeginMenu - hover-based menu with rounded corners
    inline bool BeginMenu(const char *label, bool enabled = true) {
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext &g = *GImGui;
        ImGuiStyle &style = g.Style;
        ImGuiID id = window->GetID(label);
        MenuState &menu_state = GetMenuState();

        ImVec2 pos = window->DC.CursorPos;
        ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);

        // For menu bar items (horizontal layout)
        if (window->DC.LayoutType == ImGuiLayoutType_Horizontal) {
            // Custom padding for menu bar items
            const float padding_x =
                16.0f; // Horizontal padding from text to border
            const float padding_y =
                4.0f; // Vertical padding from text to border

            // Override item spacing to place rectangles adjacent
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

            ImVec2 size(label_size.x + padding_x * 2.0f,
                label_size.y + padding_y * 2.0f);
            ImRect bb(pos, pos + size);
            ImGui::ItemSize(size, padding_y);

            ImGui::PopStyleVar();

            if (!ImGui::ItemAdd(bb, id))
                return false;

            ImVec2 mouse_pos = ImGui::GetMousePos();
            bool item_hovered = bb.Contains(mouse_pos);
            bool is_active_menu = (menu_state.active_menu_id == id);

            // Check if we should open this menu
            if (item_hovered && enabled) {
                menu_state.active_menu_id = id;
                menu_state.active_menu_rect = bb;
                is_active_menu = true;
            }

            // Render rounded background (use WindowRounding to match popup
            // corners)
            if (is_active_menu || item_hovered) {
                ImU32 col =
                    ImGui::GetColorU32(is_active_menu ? ImGuiCol_HeaderActive
                                                      : ImGuiCol_HeaderHovered);
                window->DrawList->AddRectFilled(bb.Min,
                    bb.Max,
                    col,
                    style.WindowRounding);
            }

            // Render text
            ImU32 text_col = ImGui::GetColorU32(
                enabled ? ImGuiCol_Text : ImGuiCol_TextDisabled);
            ImVec2 text_pos = bb.Min + ImVec2(padding_x, padding_y);
            ImGui::RenderText(text_pos, label);

            // Show popup window if this is the active menu
            if (is_active_menu) {
                ImVec2 popup_pos(bb.Min.x, bb.Max.y + 2.0f);
                ImGui::SetNextWindowPos(popup_pos);
                ImGui::SetNextWindowSize(ImVec2(0, 0)); // Auto-size

                ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                                         ImGuiWindowFlags_NoResize |
                                         ImGuiWindowFlags_NoMove |
                                         ImGuiWindowFlags_AlwaysAutoResize |
                                         ImGuiWindowFlags_NoSavedSettings;

                char window_name[256];
                snprintf(window_name, sizeof(window_name), "##Menu_%s", label);

                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.9f);

                if (ImGui::Begin(window_name, nullptr, flags)) {
                    // Check if mouse is outside both menu item and popup
                    ImGuiWindow *popup_window = ImGui::GetCurrentWindow();
                    ImRect popup_rect(popup_window->Pos,
                        popup_window->Pos + popup_window->Size);

                    // Extend menu item rect downward to connect with popup
                    ImRect extended_item_rect = menu_state.active_menu_rect;
                    extended_item_rect.Max.y = popup_rect.Min.y;

                    bool in_menu_item = extended_item_rect.Contains(mouse_pos);
                    bool in_popup = popup_rect.Contains(mouse_pos);

                    if (!in_menu_item && !in_popup) {
                        menu_state.active_menu_id = 0;
                        ImGui::End();
                        ImGui::PopStyleVar();
                        return false;
                    }

                    menu_state.alpha_pushed = true;
                    return true;
                }

                ImGui::PopStyleVar();
            }
        } else {
            // Vertical layout (submenu) - use standard ImGui for now
            return ImGui::BeginMenu(label, enabled);
        }

        return false;
    }

    inline void EndMenu() {
        MenuState &menu_state = GetMenuState();
        ImGui::End();
        if (menu_state.alpha_pushed) {
            ImGui::PopStyleVar();
            menu_state.alpha_pushed = false;
        }
    }

} // namespace ui
