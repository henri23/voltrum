#include "ui_titlebar.hpp"
#include "renderer/renderer_frontend.hpp"
#include "systems/texture_system.hpp"
#include "ui_themes.hpp"
#include "ui_types.hpp"

#include "core/asserts.hpp"
#include "core/logger.hpp"

#include "platform/platform.hpp"

#include <imgui_internal.h>

void ui_titlebar_setup(UI_Context *context, const char *app_name) {
    UI_Titlebar_State *state = &context->titlebar;

    state->title_text = app_name;

    state->app_icon_texture =
        texture_system_acquire("voltrum_icon", false, true);

    state->minimize_icon_texture =
        texture_system_acquire("window_minimize_icon", false, true);

    state->maximize_icon_texture =
        texture_system_acquire("window_maximize_icon", false, true);

    state->restore_icon_texture =
        texture_system_acquire("window_restore_icon", false, true);

    state->close_icon_texture =
        texture_system_acquire("window_close_icon", false, true);

    RUNTIME_ASSERT_MSG(state->app_icon_texture, "Failed to load app icon");

    RUNTIME_ASSERT_MSG(state->minimize_icon_texture,
        "Failed to load minimize icon");

    RUNTIME_ASSERT_MSG(state->maximize_icon_texture,
        "Failed to load maximize icon");

    RUNTIME_ASSERT_MSG(state->restore_icon_texture,
        "Failed to load restore icon");

    RUNTIME_ASSERT_MSG(state->close_icon_texture, "Failed to load close icon");

    CORE_INFO("Titlebar icons loaded successfully");
}

void ui_titlebar_draw(UI_Context *context) {
    UI_Titlebar_State *state = &context->titlebar;
    const UI_Theme_Palette &palette =
        ui_themes_get_palette(context->current_theme);

    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImVec2 window_pos = viewport->Pos;
    ImVec2 window_size = viewport->Size;

    state->titlebar_min = window_pos;
    state->titlebar_max =
        ImVec2(window_pos.x + window_size.x, window_pos.y + TITLEBAR_HEIGHT);

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(window_size.x, TITLEBAR_HEIGHT));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoBackground;

    // Draw titlebar background to background draw list (behind popups)
    ImDrawList *bg_draw_list = ImGui::GetBackgroundDrawList(viewport);
    f32 titlebar_width = state->titlebar_max.x - state->titlebar_min.x;

    bg_draw_list->AddRectFilled(state->titlebar_min,
        state->titlebar_max,
        palette.titlebar);

    f32 gradient_width = titlebar_width * 0.25f;
    ImVec2 gradient_min = state->titlebar_min;
    ImVec2 gradient_max =
        ImVec2(state->titlebar_min.x + gradient_width, state->titlebar_max.y);

    bg_draw_list->AddRectFilledMultiColor(gradient_min,
        gradient_max,
        palette.titlebar_gradient_start,
        palette.titlebar_gradient_end,
        palette.titlebar_gradient_end,
        palette.titlebar_gradient_start);

    const f32 top_padding = 4.0f;
    f32 logo_size = 50.0f;
    f32 logo_margin = 4.0f;
    ImVec2 logo_pos = ImVec2(state->titlebar_min.x + logo_margin,
        state->titlebar_min.y + top_padding);

    ImTextureID app_icon_id =
        (ImTextureID)renderer_get_texture_draw_data(state->app_icon_texture);
    bg_draw_list->AddImage(app_icon_id,
        logo_pos,
        ImVec2(logo_pos.x + logo_size, logo_pos.y + logo_size),
        ImVec2(0, 1),
        ImVec2(1, 0),
        IM_COL32_WHITE);

    if (ImGui::Begin("##CustomTitlebar", nullptr, flags)) {
        ImGui::GetCurrentWindow()->DockNode = nullptr;
        ImDrawList *draw_list = ImGui::GetWindowDrawList();

        // Menu bar
        {
            f32 menu_x = logo_margin + logo_size + 8.0f;
            f32 menu_y = top_padding;
            ImGui::SetCursorPos(ImVec2(menu_x, menu_y));

            ImVec2 menu_start = ImGui::GetCursorScreenPos();
            ImGuiWindow *window = ImGui::GetCurrentWindow();
            const ImVec2 padding = window->WindowPadding;

            // Setup menubar state (matching Walnut's BeginMenubar)
            ImGui::BeginGroup();
            ImGui::PushID("##menubar");

            // Calculate bar rect and clip rect
            ImRect bar_rect(menu_start,
                ImVec2(menu_start.x + window_size.x,
                    menu_start.y + ImGui::GetFrameHeightWithSpacing()));
            ImRect clip_rect(IM_ROUND(bar_rect.Min.x),
                IM_ROUND(bar_rect.Min.y),
                IM_ROUND(bar_rect.Max.x),
                IM_ROUND(bar_rect.Max.y));
            clip_rect.ClipWith(window->OuterRectClipped);
            ImGui::PushClipRect(clip_rect.Min, clip_rect.Max, false);

            window->DC.CursorPos = window->DC.CursorMaxPos = menu_start;
            window->DC.LayoutType = ImGuiLayoutType_Horizontal;
            window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
            window->DC.MenuBarAppending = true;
            ImGui::AlignTextToFramePadding();

            if (context->menu_callback) {
                // Push smaller horizontal padding for menu items
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                    ImVec2(5.0f, 5.0f));
                context->menu_callback();
                ImGui::PopStyleVar();
            }

            // Track menu bar end position for hover detection
            f32 menu_bar_end_x = window->DC.CursorPos.x;

            // Cleanup menubar state (matching Walnut's EndMenubar)
            ImGui::PopClipRect();
            ImGui::PopID();
            window->DC.MenuBarOffset.x = window->DC.CursorPos.x - window->Pos.x;
            GImGui->GroupStack.back().EmitItem = false;
            ImGui::EndGroup();
            window->DC.LayoutType = ImGuiLayoutType_Vertical;
            window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
            window->DC.MenuBarAppending = false;

            // Check if menu is hovered for titlebar drag exclusion
            ImVec2 mouse_pos = ImGui::GetMousePos();
            state->is_menu_hovered =
                mouse_pos.x >= menu_start.x && mouse_pos.x <= menu_bar_end_x &&
                mouse_pos.y >= menu_start.y &&
                mouse_pos.y <=
                    menu_start.y + ImGui::GetFrameHeightWithSpacing();

            // Close menu popups when clicking outside the menu area
            if (ImGui::IsMouseClicked(0) && !state->is_menu_hovered &&
                !ImGui::IsAnyItemHovered()) {
                // Close any open popups by focusing the main window
                ImGui::FocusWindow(window);
            }
        }

        if (state->title_text) {
            ImVec2 text_size = ImGui::CalcTextSize(state->title_text);
            f32 text_x =
                state->titlebar_min.x + (titlebar_width - text_size.x) * 0.5f;
            f32 text_y = state->titlebar_min.y + top_padding;
            ImVec2 text_pos = ImVec2(text_x, text_y);
            draw_list->AddText(text_pos, palette.text, state->title_text);
        }

        f32 button_size = 26.0f;
        f32 button_spacing = 2.0f;
        f32 right_margin = 4.0f;

        f32 current_x = state->titlebar_max.x - right_margin;

        current_x -= button_size;
        ImVec2 close_pos =
            ImVec2(current_x, state->titlebar_min.y + top_padding);
        ImGui::SetCursorScreenPos(close_pos);
        ImGui::PushID("close");
        b8 close_clicked = ImGui::InvisibleButton("##close_btn",
            ImVec2(button_size, button_size));
        u32 close_color = palette.titlebar;
        if (ImGui::IsItemActive()) {
            close_color = palette.highlight;
        } else if (ImGui::IsItemHovered()) {
            close_color = palette.button_hovered;
        }
        draw_list->AddRectFilled(close_pos,
            ImVec2(close_pos.x + button_size, close_pos.y + button_size),
            close_color);

        ImTextureID close_icon_id = (ImTextureID)renderer_get_texture_draw_data(
            state->close_icon_texture);
        f32 icon_size = button_size * 0.6f;
        ImVec2 icon_pos = ImVec2(close_pos.x + (button_size - icon_size) * 0.5f,
            close_pos.y + (button_size - icon_size) * 0.5f);
        u32 icon_color = ImGui::IsItemHovered() ? IM_COL32_WHITE : palette.text;
        draw_list->AddImage(close_icon_id,
            icon_pos,
            ImVec2(icon_pos.x + icon_size, icon_pos.y + icon_size),
            ImVec2(0, 1),
            ImVec2(1, 0),
            icon_color);
        if (close_clicked) {
            platform_close_window();
        }
        ImGui::PopID();

        current_x -= button_spacing;
        current_x -= button_size;
        b8 is_maximized = platform_is_window_maximized();
        Texture *max_icon = is_maximized ? state->restore_icon_texture
                                         : state->maximize_icon_texture;
        ImVec2 max_pos = ImVec2(current_x, state->titlebar_min.y + top_padding);
        ImGui::SetCursorScreenPos(max_pos);
        ImGui::PushID("maximize");
        b8 max_clicked = ImGui::InvisibleButton("##max_btn",
            ImVec2(button_size, button_size));
        u32 max_color = palette.titlebar;
        if (ImGui::IsItemActive()) {
            max_color = palette.highlight;
        } else if (ImGui::IsItemHovered()) {
            max_color = palette.button_hovered;
        }
        draw_list->AddRectFilled(max_pos,
            ImVec2(max_pos.x + button_size, max_pos.y + button_size),
            max_color);

        ImTextureID max_icon_id =
            (ImTextureID)renderer_get_texture_draw_data(max_icon);
        f32 max_icon_size = button_size * 0.6f;
        ImVec2 max_icon_pos =
            ImVec2(max_pos.x + (button_size - max_icon_size) * 0.5f,
                max_pos.y + (button_size - max_icon_size) * 0.5f);
        u32 max_icon_color =
            ImGui::IsItemHovered() ? IM_COL32_WHITE : palette.text;
        draw_list->AddImage(max_icon_id,
            max_icon_pos,
            ImVec2(max_icon_pos.x + max_icon_size,
                max_icon_pos.y + max_icon_size),
            ImVec2(0, 1),
            ImVec2(1, 0),
            max_icon_color);
        if (max_clicked) {
            if (is_maximized) {
                platform_restore_window();
            } else {
                platform_maximize_window();
            }
        }
        ImGui::PopID();

        current_x -= button_spacing;
        current_x -= button_size;
        ImVec2 min_pos = ImVec2(current_x, state->titlebar_min.y + top_padding);
        ImGui::SetCursorScreenPos(min_pos);
        ImGui::PushID("minimize");
        b8 min_clicked = ImGui::InvisibleButton("##min_btn",
            ImVec2(button_size, button_size));
        u32 min_color = palette.titlebar;
        if (ImGui::IsItemActive()) {
            min_color = palette.highlight;
        } else if (ImGui::IsItemHovered()) {
            min_color = palette.button_hovered;
        }
        draw_list->AddRectFilled(min_pos,
            ImVec2(min_pos.x + button_size, min_pos.y + button_size),
            min_color);

        ImTextureID min_icon_id = (ImTextureID)renderer_get_texture_draw_data(
            state->minimize_icon_texture);
        f32 min_icon_width = button_size * 0.6f;
        f32 min_icon_height = button_size * 0.15f;
        ImVec2 min_icon_pos =
            ImVec2(min_pos.x + (button_size - min_icon_width) * 0.5f,
                min_pos.y + button_size * 0.7f);
        u32 min_icon_color =
            ImGui::IsItemHovered() ? IM_COL32_WHITE : palette.text;
        draw_list->AddImage(min_icon_id,
            min_icon_pos,
            ImVec2(min_icon_pos.x + min_icon_width,
                min_icon_pos.y + min_icon_height),
            ImVec2(0, 1),
            ImVec2(1, 0),
            min_icon_color);
        if (min_clicked) {
            platform_minimize_window();
        }
        ImGui::PopID();

        // Compute titlebar hover state for window dragging
        // Titlebar is draggable when mouse is over it but NOT over interactive
        // elements
        ImVec2 mouse_pos = ImGui::GetMousePos();
        b8 in_titlebar = mouse_pos.x >= state->titlebar_min.x &&
                         mouse_pos.x <= state->titlebar_max.x &&
                         mouse_pos.y >= state->titlebar_min.y &&
                         mouse_pos.y <= state->titlebar_max.y;
        b8 any_item_hovered = ImGui::IsAnyItemHovered();
        b8 new_hover_state = in_titlebar && !any_item_hovered;
        if (new_hover_state != state->is_titlebar_hovered) {
            state->is_titlebar_hovered = new_hover_state;
            platform_set_titlebar_hovered(new_hover_state);
        }
    }
    ImGui::End();
}

b8 ui_is_titlebar_hovered(UI_Context *context) {
    return context->titlebar.is_titlebar_hovered;
}
