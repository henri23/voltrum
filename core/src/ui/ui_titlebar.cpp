#include "ui_titlebar.hpp"
#include "icons.hpp"
#include "renderer/renderer_frontend.hpp"
#include "systems/texture_system.hpp"
#include "ui_themes.hpp"
#include "ui_types.hpp"

#include "core/asserts.hpp"
#include "core/logger.hpp"

#include "platform/platform.hpp"

#include <imgui_internal.h>

void
ui_titlebar_setup(
    UI_State   *context,
    const char *logo_asset_name
)
{
    UI_Titlebar_State *state = &context->titlebar;

    state->app_icon_texture = texture_system_acquire(
        logo_asset_name,
        false,
        true
    );

    state->minimize_icon_texture = texture_system_acquire(
        "window_minimize_icon",
        false,
        true
    );

    state->maximize_icon_texture = texture_system_acquire(
        "window_maximize_icon",
        false,
        true
    );

    state->restore_icon_texture = texture_system_acquire(
        "window_restore_icon",
        false,
        true
    );

    state->close_icon_texture = texture_system_acquire(
        "window_close_icon",
        false,
        true
    );

    RUNTIME_ASSERT_MSG(state->app_icon_texture, "Failed to load logo icon");
    RUNTIME_ASSERT_MSG(state->minimize_icon_texture, "Failed to load minimize icon");
    RUNTIME_ASSERT_MSG(state->maximize_icon_texture, "Failed to load maximize icon");
    RUNTIME_ASSERT_MSG(state->restore_icon_texture, "Failed to load restore icon");
    RUNTIME_ASSERT_MSG(state->close_icon_texture, "Failed to load close icon");

    CORE_INFO("Titlebar icons loaded successfully");
}

void
ui_titlebar_draw(UI_State *context)
{
    UI_Titlebar_State      *state   = &context->titlebar;
    const UI_Theme_Palette &palette = ui_themes_get_palette(context->current_theme);

    ImGuiViewport *viewport    = ImGui::GetMainViewport();
    ImVec2         window_pos  = viewport->Pos;
    ImVec2         window_size = viewport->Size;

    state->titlebar_min = window_pos;
    state->titlebar_max = ImVec2(
        window_pos.x + window_size.x,
        window_pos.y + TITLEBAR_HEIGHT
    );

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(window_size.x, TITLEBAR_HEIGHT));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoBackground;

    // Draw titlebar background to background draw list (behind popups)
    ImDrawList *bg_draw_list   = ImGui::GetBackgroundDrawList(viewport);
    f32         titlebar_width = state->titlebar_max.x - state->titlebar_min.x;

    bg_draw_list->AddRectFilled(
        state->titlebar_min,
        state->titlebar_max,
        palette.titlebar
    );

    // Gradient on left side
    f32    gradient_width = titlebar_width * 0.25f;
    ImVec2 gradient_min   = state->titlebar_min;
    ImVec2 gradient_max   = ImVec2(
        state->titlebar_min.x + gradient_width,
        state->titlebar_max.y
    );

    bg_draw_list->AddRectFilledMultiColor(
        gradient_min,
        gradient_max,
        palette.titlebar_gradient_start,
        palette.titlebar_gradient_end,
        palette.titlebar_gradient_end,
        palette.titlebar_gradient_start
    );

    // Logo
    const f32 logo_margin = 4.0f;
    f32       logo_size   = 50.0f * UI_PLATFORM_SCALE;
    f32       logo_y      = state->titlebar_min.y + (TITLEBAR_HEIGHT - logo_size) * 0.5f;
    ImVec2    logo_pos    = ImVec2(
        state->titlebar_min.x + logo_margin,
        logo_y
    );

    ImTextureID app_icon_id = (ImTextureID)renderer_get_texture_draw_data(
        state->app_icon_texture
    );
    bg_draw_list->AddImage(
        app_icon_id,
        logo_pos,
        ImVec2(logo_pos.x + logo_size, logo_pos.y + logo_size),
        ImVec2(0, 1),
        ImVec2(1, 0),
        IM_COL32_WHITE
    );

    if (ImGui::Begin("##CustomTitlebar", nullptr, flags))
    {
        ImGui::GetCurrentWindow()->DockNode = nullptr;
        ImDrawList *draw_list               = ImGui::GetWindowDrawList();

        // Window controls as a compact cluster with custom vector icons.
        f32 button_size     = 30.0f * UI_PLATFORM_SCALE;
        f32 button_spacing  = 2.0f * UI_PLATFORM_SCALE;
        f32 button_rounding = 7.0f * UI_PLATFORM_SCALE;
        f32 right_margin    = 10.0f * UI_PLATFORM_SCALE;
        f32 cluster_padding = 2.0f * UI_PLATFORM_SCALE;
        f32 cluster_rounding = 10.0f * UI_PLATFORM_SCALE;
        f32 stroke          = 1.6f * UI_PLATFORM_SCALE;

        f32 cluster_width =
            button_size * 3.0f + button_spacing * 2.0f + cluster_padding * 2.0f;
        f32 cluster_height = button_size + cluster_padding * 2.0f;

        ImVec2 cluster_min = ImVec2(
            state->titlebar_max.x - right_margin - cluster_width,
            state->titlebar_min.y + (TITLEBAR_HEIGHT - cluster_height) * 0.5f
        );
        ImVec2 cluster_max = ImVec2(
            cluster_min.x + cluster_width,
            cluster_min.y + cluster_height
        );

        draw_list->AddRectFilled(
            cluster_min,
            cluster_max,
            IM_COL32(255, 255, 255, 14),
            cluster_rounding
        );
        draw_list->AddRect(
            cluster_min,
            cluster_max,
            IM_COL32(255, 255, 255, 26),
            cluster_rounding
        );

        // Visual separation between titlebar content and control cluster.
        f32 separator_x = cluster_min.x - 8.0f * UI_PLATFORM_SCALE;
        draw_list->AddLine(
            ImVec2(separator_x, state->titlebar_min.y + 11.0f * UI_PLATFORM_SCALE),
            ImVec2(separator_x, state->titlebar_max.y - 11.0f * UI_PLATFORM_SCALE),
            IM_COL32(255, 255, 255, 22),
            1.0f
        );

        f32 button_y = cluster_min.y + cluster_padding;
        ImVec2 min_pos = ImVec2(cluster_min.x + cluster_padding, button_y);
        ImVec2 max_pos = ImVec2(min_pos.x + button_size + button_spacing, button_y);
        ImVec2 close_pos = ImVec2(max_pos.x + button_size + button_spacing, button_y);

        const u32 neutral_hover_bg  = IM_COL32(255, 255, 255, 26);
        const u32 neutral_active_bg = IM_COL32(255, 255, 255, 42);
        // Catppuccin-friendly red family for close button (idle/hover/active).
        const u32 close_idle_bg   = IM_COL32(231, 130, 132, 190);
        const u32 close_hover_bg  = IM_COL32(243, 139, 168, 255);
        const u32 close_active_bg = IM_COL32(214, 110, 130, 255);

        // Minimize button
        ImGui::SetCursorScreenPos(min_pos);
        ImGui::PushID("minimize");
        b8 min_clicked = ImGui::InvisibleButton(
            "##min_btn",
            ImVec2(button_size, button_size)
        );
        b8 min_hovered = ImGui::IsItemHovered();
        b8 min_active  = ImGui::IsItemActive();
        if (min_hovered || min_active)
        {
            draw_list->AddRectFilled(
                min_pos,
                ImVec2(min_pos.x + button_size, min_pos.y + button_size),
                min_active ? neutral_active_bg : neutral_hover_bg,
                button_rounding
            );
        }
        {
            u32    icon_col = min_hovered ? palette.text_brighter : palette.text;
            ImVec2 c        = ImVec2(
                min_pos.x + button_size * 0.5f,
                min_pos.y + button_size * 0.5f + 3.0f * UI_PLATFORM_SCALE
            );
            f32 half = 5.0f * UI_PLATFORM_SCALE;
            draw_list->AddLine(
                ImVec2(c.x - half, c.y),
                ImVec2(c.x + half, c.y),
                icon_col,
                stroke
            );
        }
        if (min_clicked)
        {
            platform_minimize_window(context->platform);
        }
        ImGui::PopID();

        // Maximize/Restore button
        b8 is_maximized = platform_is_window_maximized(context->platform);
        ImGui::SetCursorScreenPos(max_pos);
        ImGui::PushID("maximize");
        b8 max_clicked = ImGui::InvisibleButton(
            "##max_btn",
            ImVec2(button_size, button_size)
        );
        b8 max_hovered = ImGui::IsItemHovered();
        b8 max_active  = ImGui::IsItemActive();
        if (max_hovered || max_active)
        {
            draw_list->AddRectFilled(
                max_pos,
                ImVec2(max_pos.x + button_size, max_pos.y + button_size),
                max_active ? neutral_active_bg : neutral_hover_bg,
                button_rounding
            );
        }
        {
            u32    icon_col = max_hovered ? palette.text_brighter : palette.text;
            ImVec2 c        = ImVec2(
                max_pos.x + button_size * 0.5f,
                max_pos.y + button_size * 0.5f
            );
            f32 half = 4.8f * UI_PLATFORM_SCALE;
            if (is_maximized)
            {
                ImVec2 back_min = ImVec2(c.x - half + 2.0f, c.y - half - 2.0f);
                ImVec2 back_max = ImVec2(c.x + half + 2.0f, c.y + half - 2.0f);
                ImVec2 front_min = ImVec2(c.x - half - 2.0f, c.y - half + 2.0f);
                ImVec2 front_max = ImVec2(c.x + half - 2.0f, c.y + half + 2.0f);
                draw_list->AddRect(back_min, back_max, icon_col, 2.0f, 0, stroke);
                draw_list->AddRect(front_min, front_max, icon_col, 2.0f, 0, stroke);
            }
            else
            {
                draw_list->AddRect(
                    ImVec2(c.x - half, c.y - half),
                    ImVec2(c.x + half, c.y + half),
                    icon_col,
                    2.0f,
                    0,
                    stroke
                );
            }
        }
        if (max_clicked)
        {
            if (is_maximized)
            {
                platform_restore_window(context->platform);
            }
            else
            {
                platform_maximize_window(context->platform);
            }
        }
        ImGui::PopID();

        // Close button
        ImGui::SetCursorScreenPos(close_pos);
        ImGui::PushID("close");
        b8 close_clicked = ImGui::InvisibleButton(
            "##close_btn",
            ImVec2(button_size, button_size)
        );
        b8 close_hovered = ImGui::IsItemHovered();
        b8 close_active  = ImGui::IsItemActive();
        u32 close_bg = close_idle_bg;
        if (close_hovered)
            close_bg = close_hover_bg;
        if (close_active)
            close_bg = close_active_bg;
        draw_list->AddRectFilled(
            close_pos,
            ImVec2(close_pos.x + button_size, close_pos.y + button_size),
            close_bg,
            button_rounding
        );
        {
            u32    icon_col = (close_hovered || close_active) ? IM_COL32_WHITE : palette.text;
            ImVec2 c        = ImVec2(
                close_pos.x + button_size * 0.5f,
                close_pos.y + button_size * 0.5f
            );
            f32 half = 4.6f * UI_PLATFORM_SCALE;
            draw_list->AddLine(
                ImVec2(c.x - half, c.y - half),
                ImVec2(c.x + half, c.y + half),
                icon_col,
                stroke
            );
            draw_list->AddLine(
                ImVec2(c.x - half, c.y + half),
                ImVec2(c.x + half, c.y - half),
                icon_col,
                stroke
            );
        }
        if (close_clicked)
        {
            platform_close_window();
        }
        ImGui::PopID();

        // Store button area for SDL hit test exclusion
        state->button_area_min = min_pos;
        state->button_area_max = ImVec2(
            close_pos.x + button_size,
            close_pos.y + button_size
        );

        f32 scale = context->platform->main_scale;
        context->platform->button_area_min_x = state->button_area_min.x * scale;
        context->platform->button_area_max_x = state->button_area_max.x * scale;
        context->platform->button_area_min_y = state->button_area_min.y * scale;
        context->platform->button_area_max_y = state->button_area_max.y * scale;

        // Compute content bounds for client callback
        f32 content_start_x = logo_margin + logo_size + 8.0f;
        f32 content_end_x   = cluster_min.x - 10.0f * UI_PLATFORM_SCALE;

        state->content_bounds.x      = state->titlebar_min.x + content_start_x;
        state->content_bounds.y      = state->titlebar_min.y;
        state->content_bounds.width  = content_end_x - content_start_x;
        state->content_bounds.height = TITLEBAR_HEIGHT;

        // Call client titlebar content callback
        if (context->titlebar_content_callback)
        {
            context->titlebar_content_callback(
                context->global_client_state,
                state->content_bounds,
                palette
            );
        }

        // Update menu hover state from content bounds
        ImVec2 mouse_pos       = ImGui::GetMousePos();
        state->is_menu_hovered =
            mouse_pos.x >= state->content_bounds.x &&
            mouse_pos.x <= state->content_bounds.x + state->content_bounds.width &&
            mouse_pos.y >= state->content_bounds.y &&
            mouse_pos.y <= state->content_bounds.y + ImGui::GetFrameHeightWithSpacing();

        // Compute titlebar hover state for window dragging
        b8 in_titlebar = mouse_pos.x >= state->titlebar_min.x &&
                         mouse_pos.x <= state->titlebar_max.x &&
                         mouse_pos.y >= state->titlebar_min.y &&
                         mouse_pos.y <= state->titlebar_max.y;
        b8 any_item_hovered    = ImGui::IsAnyItemHovered();
        state->is_titlebar_hovered = in_titlebar && !any_item_hovered;

        // Block OS-level titlebar drag when an ImGui window overlaps
        ImGuiWindow *hovered_window  = GImGui->HoveredWindow;
        ImGuiWindow *titlebar_window = ImGui::GetCurrentWindow();
        b8 imgui_blocks_drag = in_titlebar && hovered_window != nullptr &&
                               hovered_window != titlebar_window;
        context->platform->block_titlebar_drag = imgui_blocks_drag;
    }
    ImGui::End();
}
