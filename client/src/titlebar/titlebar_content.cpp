#include "titlebar_content.hpp"
#include "global_client_state.hpp"

#include <platform/platform.hpp>
#include <ui/icons.hpp>
#include <ui/ui_widgets.hpp>
#include <utils/string.hpp>

#include <imgui.h>
#include <imgui_internal.h>

internal_var const Client_Mode TITLEBAR_MODE_VALUES[] = {
    Client_Mode::SCHEMATIC,
    Client_Mode::LAYOUT,
    Client_Mode::SYMBOL,
    Client_Mode::SIMULATION};

internal_var const ui::icon_selector_item TITLEBAR_MODE_ITEMS[] = {
    {STR_LIT(ICON_FA_DIAGRAM_PROJECT),
     STR_LIT("SCHEMATIC"),
     STR_LIT("Schematic Mode")},
    {STR_LIT(ICON_FA_RULER_COMBINED),
     STR_LIT("LAYOUT"),
     STR_LIT("Layout Mode")},
    {STR_LIT(ICON_FA_SHAPES), STR_LIT("SYMBOL"), STR_LIT("Symbol Mode")},
    {STR_LIT(ICON_FA_WAVE_SQUARE),
     STR_LIT("SIMULATION"),
     STR_LIT("Simulation Mode")}};

static_assert(ARRAY_COUNT(TITLEBAR_MODE_VALUES) ==
                  ARRAY_COUNT(TITLEBAR_MODE_ITEMS),
              "Titlebar mode value and visual arrays must match");

INTERNAL_FUNC void
titlebar_render_mode_selector(Global_Client_State    *g_state,
                              const UI_Theme_Palette *palette,
                              const f32               menu_height,
                              const vec2              content_area_min,
                              const vec2              content_area_max)
{
    if (!g_state || !palette)
    {
        return;
    }

    const f32 content_height      = content_area_max.y - content_area_min.y;
    const f32 before_spacing      = 14.0f;
    f32       selector_height     = IM_ROUND(menu_height + 10.0f);
    const f32 min_selector_height = IM_ROUND(menu_height + 6.0f);
    const f32 max_selector_height =
        ImMax(min_selector_height, IM_ROUND(content_height - 4.0f));
    selector_height =
        ImClamp(selector_height, min_selector_height, max_selector_height);

    s32 active_index = 0;
    for (u32 i = 0; i < ARRAY_COUNT(TITLEBAR_MODE_ITEMS); ++i)
    {
        if (TITLEBAR_MODE_VALUES[i] == g_state->mode)
        {
            active_index = (s32)i;
            break;
        }
    }

    const f32 container_x = ImGui::GetCursorScreenPos().x + before_spacing;
    const f32 container_y = IM_ROUND(content_area_min.y +
                                     (content_height - selector_height) * 0.5f);

    ImGui::SetCursorScreenPos(ImVec2(container_x, container_y));
    ui::icon_selector_overrides selector_overrides = {};
    selector_overrides.icon_gap                    = 7.0f;
    selector_overrides.horizontal_padding          = 4.0f;
    selector_overrides.vertical_padding            = 4.0f;
    selector_overrides.container_bg_alpha          = 0.55f;
    selector_overrides.container_border_alpha      = 0.65f;
    selector_overrides.hover_overlay_alpha         = 0.35f;
    selector_overrides.active_outline_alpha        = 0.70f;
    selector_overrides.active_anim_sharpness       = 33.0f;
    selector_overrides.show_active_label           = 1;
    selector_overrides.active_text_gap             = 8.0f;
    selector_overrides.active_text_padding         = 11.0f;
    selector_overrides.tooltip_border_alpha        = 0.82f;

    if (ui::icon_selector("titlebar_mode_selector",
                          TITLEBAR_MODE_ITEMS,
                          (s32)ARRAY_COUNT(TITLEBAR_MODE_ITEMS),
                          &active_index,
                          selector_height,
                          *palette,
                          &selector_overrides))
    {
        g_state->mode = TITLEBAR_MODE_VALUES[active_index];
    }
}

void
client_titlebar_content_callback(void                   *client_state,
                                 vec2                    content_area_min,
                                 vec2                    content_area_max,
                                 const UI_Theme_Palette *palette)
{
    auto g_state = (Global_Client_State *)client_state;

    ImGuiWindow *window         = ImGui::GetCurrentWindow();
    const f32    menu_height    = ImGui::GetTextLineHeight() + 8.0f;
    const f32    content_height = content_area_max.y - content_area_min.y;
    ImVec2       menu_start =
        ImVec2(content_area_min.x,
               content_area_min.y + (content_height - menu_height) * 0.5f);

    ImGui::SetCursorScreenPos(menu_start);

    ImGui::BeginGroup();
    ImGui::PushID("##titlebar_menus");

    ImRect bar_rect(ImVec2(content_area_min.x, content_area_min.y),
                    ImVec2(content_area_max.x, content_area_max.y));
    ImRect clip_rect(IM_ROUND(bar_rect.Min.x),
                     IM_ROUND(bar_rect.Min.y),
                     IM_ROUND(bar_rect.Max.x),
                     IM_ROUND(bar_rect.Max.y));
    clip_rect.ClipWith(window->OuterRectClipped);
    ImGui::PushClipRect(clip_rect.Min, clip_rect.Max, false);

    window->DC.CursorPos = window->DC.CursorMaxPos = menu_start;
    window->DC.LayoutType                          = ImGuiLayoutType_Horizontal;
    window->DC.NavLayerCurrent                     = ImGuiNavLayer_Menu;
    window->DC.MenuBarAppending                    = true;
    ImGui::AlignTextToFramePadding();

    if (ui::begin_menu("FILE"))
    {
        if (ui::menu_item(ICON_FA_RIGHT_FROM_BRACKET " Exit"))
        {
            platform_close_window();
        }
        ui::end_menu();
    }

    if (ui::begin_menu("VIEW"))
    {
        ui::menu_item(ICON_FA_WINDOW_MAXIMIZE " Viewport");
        ui::menu_item(ICON_FA_SLIDERS " Properties");
        ImGui::Separator();
        ui::menu_item(ICON_FA_CODE " ImGui Demo",
                      nullptr,
                      &g_state->is_imgui_demo_visible);
        ui::menu_item(ICON_FA_CHART_LINE " ImPlot Demo",
                      nullptr,
                      &g_state->is_implot_demo_visible);
        ui::end_menu();
    }

    if (ui::begin_menu("HELP"))
    {
        ui::menu_item(ICON_FA_CIRCLE_INFO " About");
        ui::end_menu();
    }

    if (ui::begin_menu("TOOLS"))
    {
        b8 was_open = g_state->is_command_palette_open;
        if (ui::menu_item(ICON_FA_TERMINAL " Command Palette",
                          "Ctrl+K",
                          &g_state->is_command_palette_open))
        {
            if (!was_open && g_state->is_command_palette_open)
            {
                g_state->request_open_command_palette  = true;
                g_state->request_close_command_palette = false;
            }
            else if (was_open && !g_state->is_command_palette_open)
            {
                g_state->request_close_command_palette = true;
                g_state->request_open_command_palette  = false;
            }
        }
        ImGui::Separator();
        ui::menu_item(ICON_FA_GEARS " Explore");
        ui::end_menu();
    }

#ifdef DEBUG_BUILD
    if (ui::begin_menu("DEBUG"))
    {
        ui::menu_item(ICON_FA_BUG " Memory Inspector",
                      "F12",
                      &g_state->is_debug_layer_visible);
        ui::end_menu();
    }
#endif

    titlebar_render_mode_selector(g_state,
                                  palette,
                                  menu_height,
                                  content_area_min,
                                  content_area_max);

    ImGui::PopClipRect();
    ImGui::PopID();

    window->DC.MenuBarOffset.x         = window->DC.CursorPos.x - window->Pos.x;
    GImGui->GroupStack.back().EmitItem = false;
    ImGui::EndGroup();
    window->DC.LayoutType       = ImGuiLayoutType_Vertical;
    window->DC.NavLayerCurrent  = ImGuiNavLayer_Main;
    window->DC.MenuBarAppending = false;
}
