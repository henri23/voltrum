#pragma once

#include "defines.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

// Custom menu widgets with rounded corners

namespace ui {

    // Rounded MenuItem - same API as ImGui::MenuItem but with rounded hover
    inline bool MenuItem(const char *label,
        const char *shortcut = nullptr,
        bool selected = false,
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

        float min_w = window->DC.MenuColumns.TotalWidth + shortcut_w;
        float w = ImMax(label_size.x + style.FramePadding.x * 2.0f + shortcut_w,
            min_w);

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

        // Render rounded background on hover (use WindowRounding for
        // consistency)
        if (hovered && enabled) {
            ImU32 col = ImGui::GetColorU32(
                held ? ImGuiCol_HeaderActive : ImGuiCol_HeaderHovered);
            window->DrawList->AddRectFilled(bb.Min,
                bb.Max,
                col,
                style.WindowRounding);
        }

        // Render checkmark for selected items
        if (selected) {
            ImU32 col = ImGui::GetColorU32(
                enabled ? ImGuiCol_Text : ImGuiCol_TextDisabled);
            float check_sz = g.FontSize;
            float pad = ImMax(1.0f, (float)(int)(check_sz / 6.0f));
            ImGui::RenderCheckMark(window->DrawList,
                bb.Min +
                    ImVec2(style.FramePadding.x, style.FramePadding.y + pad),
                col,
                check_sz - pad * 2.0f);
        }

        // Render text
        ImU32 text_col =
            ImGui::GetColorU32(enabled ? ImGuiCol_Text : ImGuiCol_TextDisabled);
        ImVec2 text_pos = bb.Min + style.FramePadding;
        ImGui::RenderText(text_pos, label);

        // Render shortcut
        if (shortcut) {
            ImU32 shortcut_col = ImGui::GetColorU32(ImGuiCol_TextDisabled);
            ImGui::RenderText(
                ImVec2(bb.Max.x - style.FramePadding.x - shortcut_size.x,
                    text_pos.y),
                shortcut);
        }

        // Close popup when menu item is activated
        if (pressed && enabled) {
            ImGui::CloseCurrentPopup();
        }

        return pressed && enabled;
    }

    // Storage for menu hover state
    struct MenuState {
        ImGuiID active_menu_id = 0;
        ImRect active_menu_rect = {};
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
                        return false;
                    }

                    return true;
                }
            }
        } else {
            // Vertical layout (submenu) - use standard ImGui for now
            return ImGui::BeginMenu(label, enabled);
        }

        return false;
    }

    inline void EndMenu() { ImGui::End(); }

} // namespace ui
