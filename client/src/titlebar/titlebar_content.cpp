#include "titlebar_content.hpp"
#include "../global_client_state.hpp"

#include <platform/platform.hpp>
#include <ui/ui_animation.hpp>
#include <ui/icons.hpp>
#include <ui/ui_widgets.hpp>
#include <utils/string.hpp>

#include <imgui.h>
#include <imgui_internal.h>

struct Titlebar_Mode_Item
{
    Client_Mode mode;
    String      icon;
    String      label;
    String      tooltip;
};

internal_var const Titlebar_Mode_Item TITLEBAR_MODE_ITEMS[] = {
    {Client_Mode::SCHEMATIC,
     STR_LIT(ICON_FA_DIAGRAM_PROJECT),
     STR_LIT("SCHEMATIC"),
     STR_LIT("Schematic Mode")},
    {Client_Mode::LAYOUT,
     STR_LIT(ICON_FA_RULER_COMBINED),
     STR_LIT("LAYOUT"),
     STR_LIT("Layout Mode")},
    {Client_Mode::SYMBOL,
     STR_LIT(ICON_FA_SHAPES),
     STR_LIT("SYMBOL"),
     STR_LIT("Symbol Mode")},
    {Client_Mode::SIMULATION,
     STR_LIT(ICON_FA_WAVE_SQUARE),
     STR_LIT("SIMULATION"),
     STR_LIT("Simulation Mode")}};

INTERNAL_FUNC void
titlebar_render_mode_selector(Global_Client_State       *g_state,
                              const UI_Theme_Palette    &palette,
                              ImGuiWindow               *window,
                              const f32                  menu_height,
                              const vec2                 content_area_min,
                              const vec2                 content_area_max)
{
    if (!g_state || !window)
    {
        return;
    }

    const ImGuiStyle &style = ImGui::GetStyle();
    const f32         rounding = style.FrameRounding;
    const f32         active_outline_thickness =
        (style.FrameBorderSize > 0.0f) ? (style.FrameBorderSize + 0.8f) : 1.8f;
    const f32 content_height = content_area_max.y - content_area_min.y;
    const f32 container_pad  = 4.0f;
    const f32 before_spacing = 14.0f;
    const f32 icon_gap       = 7.0f;
    const f32 active_text_gap = 8.0f;
    const f32 active_text_pad = 11.0f;

    f32 slot_size = IM_ROUND(menu_height + 2.0f);
    const f32 min_slot_size = IM_ROUND(menu_height);
    const f32 max_slot_size =
        IM_ROUND(content_height - container_pad * 2.0f - 4.0f);
    if (slot_size > max_slot_size)
    {
        slot_size = max_slot_size;
    }
    if (slot_size < min_slot_size)
    {
        slot_size = min_slot_size;
    }

    s32 active_index = 0;
    for (u32 i = 0; i < ARRAY_COUNT(TITLEBAR_MODE_ITEMS); ++i)
    {
        if (TITLEBAR_MODE_ITEMS[i].mode == g_state->mode)
        {
            active_index = (s32)i;
            break;
        }
    }

    f32 max_label_width = 0.0f;
    for (u32 i = 0; i < ARRAY_COUNT(TITLEBAR_MODE_ITEMS); ++i)
    {
        const f32 label_width = ImGui::CalcTextSize(C_STR(TITLEBAR_MODE_ITEMS[i].label)).x;
        if (label_width > max_label_width)
        {
            max_label_width = label_width;
        }
    }

    const Titlebar_Mode_Item &active_item = TITLEBAR_MODE_ITEMS[active_index];
    const f32 inactive_slot_width = slot_size;
    const f32 active_slot_width = slot_size + active_text_gap + max_label_width +
                                  active_text_pad * 2.0f;

    f32 strip_width = 0.0f;
    for (u32 i = 0; i < ARRAY_COUNT(TITLEBAR_MODE_ITEMS); ++i)
    {
        strip_width +=
            (i == (u32)active_index) ? active_slot_width : inactive_slot_width;
    }
    strip_width += icon_gap * (f32)(ARRAY_COUNT(TITLEBAR_MODE_ITEMS) - 1);

    const f32 container_width  = container_pad * 2.0f + strip_width;
    const f32 container_height = container_pad * 2.0f + slot_size;

    const f32 container_x = ImGui::GetCursorScreenPos().x + before_spacing;
    const f32 container_y =
        IM_ROUND(content_area_min.y + (content_height - container_height) * 0.5f);

    ImGui::SetCursorScreenPos(ImVec2(container_x, container_y));
    ImVec2 container_pos = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(container_width, container_height));

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(container_pos,
                             ImVec2(container_pos.x + container_width,
                                    container_pos.y + container_height),
                             ui::with_alpha(palette.window_bg, 0.55f),
                             rounding);
    draw_list->AddRect(container_pos,
                       ImVec2(container_pos.x + container_width,
                              container_pos.y + container_height),
                       ui::with_alpha(palette.group_header, 0.65f),
                       rounding);

    f32 active_target_x = container_pos.x + container_pad;
    for (s32 i = 0; i < active_index; ++i)
    {
        active_target_x += inactive_slot_width + icon_gap;
    }
    ImGuiStorage *storage = &window->StateStorage;
    f32          *active_x = storage->GetFloatRef(
                 ImGui::GetID("titlebar_mode_active_x"),
                 active_target_x);
    *active_x = ui::anim::exp_decay_to(*active_x,
                                       active_target_x,
                                       28.0f,
                                       ImGui::GetIO().DeltaTime);

    const ImVec2 active_min = ImVec2(*active_x, container_pos.y + container_pad);
    const ImVec2 active_max =
        ImVec2(active_min.x + active_slot_width, active_min.y + slot_size);
    draw_list->AddRectFilled(active_min, active_max, palette.tab_active, rounding);
    draw_list->AddRect(active_min,
                       active_max,
                       ui::with_alpha(palette.accent, 0.70f),
                       rounding,
                       0,
                       active_outline_thickness);

    ImGui::PushID("titlebar_mode_selector");
    f32 slot_x = container_pos.x + container_pad;
    for (u32 i = 0; i < ARRAY_COUNT(TITLEBAR_MODE_ITEMS); ++i)
    {
        const Titlebar_Mode_Item &item = TITLEBAR_MODE_ITEMS[i];
        const b8 active = item.mode == g_state->mode;
        const f32 slot_width = active ? active_slot_width : inactive_slot_width;
        ImVec2 button_min = ImVec2(slot_x, container_pos.y + container_pad);

        ImGui::SetCursorScreenPos(button_min);
        ImGui::PushID((s32)i);
        if (ImGui::InvisibleButton("##mode_button",
                                   ImVec2(slot_width, slot_size)))
        {
            g_state->mode = item.mode;
        }

        const b8 hovered = ImGui::IsItemHovered();

        if (hovered && !active)
        {
            draw_list->AddRectFilled(button_min,
                                     ImVec2(button_min.x + slot_width,
                                            button_min.y + slot_size),
                                     ui::with_alpha(palette.text_darker, 0.35f),
                                     rounding);
        }

        if (hovered)
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(C_STR(item.tooltip),
                                   C_STR(item.tooltip) + item.tooltip.size);
            ImGui::EndTooltip();
        }

        const ImVec2 icon_size = ImGui::CalcTextSize(C_STR(item.icon));
        ImVec2 icon_pos        = ImVec2(0.0f, 0.0f);
        ImU32  icon_color      = palette.text;
        if (active)
        {
            icon_pos = ImVec2(button_min.x + active_text_pad,
                              button_min.y + (slot_size - icon_size.y) * 0.5f);
            icon_color = palette.text_brighter;
            draw_list->AddText(icon_pos, icon_color, C_STR(item.icon));

            const ImVec2 label_size = ImGui::CalcTextSize(C_STR(item.label));
            const ImVec2 label_pos =
                ImVec2(icon_pos.x + icon_size.x + active_text_gap,
                       button_min.y + (slot_size - label_size.y) * 0.5f);
            draw_list->AddText(label_pos, palette.text_brighter, C_STR(item.label));
        }
        else
        {
            icon_pos = ImVec2(button_min.x + (slot_size - icon_size.x) * 0.5f,
                              button_min.y + (slot_size - icon_size.y) * 0.5f);
            draw_list->AddText(icon_pos, icon_color, C_STR(item.icon));
        }

        ImGui::PopID();
        slot_x += slot_width + icon_gap;
    }
    ImGui::PopID();
}

void
client_titlebar_content_callback(void                   *client_state,
                                 vec2                    content_area_min,
                                 vec2                    content_area_max,
                                 const UI_Theme_Palette &palette)
{
    auto g_state = (Global_Client_State *)client_state;

    ImGuiWindow *window      = ImGui::GetCurrentWindow();
    const f32    menu_height = ImGui::GetTextLineHeight() + 8.0f;
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
                                  window,
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
