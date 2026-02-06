#include "titlebar_content.hpp"
#include "../global_client_state.hpp"

#include <ui/icons.hpp>
#include <ui/ui_widgets.hpp>
#include <platform/platform.hpp>

#include <imgui.h>
#include <imgui_internal.h>

void
client_titlebar_content_callback(
    void                          *client_state,
    const Titlebar_Content_Bounds &bounds,
    const UI_Theme_Palette        &palette
)
{
    auto g_state = (Global_Client_State *)client_state;

    ImGuiWindow *window = ImGui::GetCurrentWindow();
    ImVec2 menu_start = ImVec2(bounds.x, bounds.y);

    ImGui::SetCursorScreenPos(menu_start);

    ImGui::BeginGroup();
    ImGui::PushID("##titlebar_menus");

    ImRect bar_rect(
        menu_start,
        ImVec2(menu_start.x + bounds.width, menu_start.y + bounds.height)
    );
    ImRect clip_rect(
        IM_ROUND(bar_rect.Min.x),
        IM_ROUND(bar_rect.Min.y),
        IM_ROUND(bar_rect.Max.x),
        IM_ROUND(bar_rect.Max.y)
    );
    clip_rect.ClipWith(window->OuterRectClipped);
    ImGui::PushClipRect(clip_rect.Min, clip_rect.Max, false);

    window->DC.CursorPos        = window->DC.CursorMaxPos = menu_start;
    window->DC.LayoutType       = ImGuiLayoutType_Horizontal;
    window->DC.NavLayerCurrent  = ImGuiNavLayer_Menu;
    window->DC.MenuBarAppending = true;
    ImGui::AlignTextToFramePadding();

    if (ui::BeginMenu("FILE"))
    {
        if (ui::MenuItem(ICON_FA_RIGHT_FROM_BRACKET " Exit"))
        {
            platform_close_window();
        }
        ui::EndMenu();
    }

    if (ui::BeginMenu("VIEW"))
    {
        ui::MenuItem(ICON_FA_WINDOW_MAXIMIZE " Viewport");
        ui::MenuItem(ICON_FA_SLIDERS " Properties");
        ImGui::Separator();
        ui::MenuItem(ICON_FA_CODE " ImGui Demo",
                     nullptr,
                     &g_state->is_imgui_demo_visible);
        ui::MenuItem(ICON_FA_CHART_LINE " ImPlot Demo",
                     nullptr,
                     &g_state->is_implot_demo_visible);
        ui::EndMenu();
    }

    if (ui::BeginMenu("HELP"))
    {
        ui::MenuItem(ICON_FA_CIRCLE_INFO " About");
        ui::EndMenu();
    }

    if (ui::BeginMenu("TOOLS"))
    {
        ui::MenuItem(ICON_FA_GEARS " Explore");
        ui::EndMenu();
    }

#ifdef DEBUG_BUILD
    if (ui::BeginMenu("DEBUG"))
    {
        ui::MenuItem(ICON_FA_BUG " Memory Inspector",
                     "F12",
                     &g_state->is_debug_layer_visible);
        ui::EndMenu();
    }
#endif

    ImGui::PopClipRect();
    ImGui::PopID();

    window->DC.MenuBarOffset.x      = window->DC.CursorPos.x - window->Pos.x;
    GImGui->GroupStack.back().EmitItem = false;
    ImGui::EndGroup();
    window->DC.LayoutType       = ImGuiLayoutType_Vertical;
    window->DC.NavLayerCurrent  = ImGuiNavLayer_Main;
    window->DC.MenuBarAppending = false;
}
