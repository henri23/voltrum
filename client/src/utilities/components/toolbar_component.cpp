#include "toolbar_component.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <ui/ui_animation.hpp>
#include <ui/icons.hpp>
#include <ui/ui_widgets.hpp>
#include <utils/string.hpp>

struct Toolbar_Tool_Item
{
    String icon;
    String tooltip;
};

internal_var const Toolbar_Tool_Item TOOLBAR_ITEMS[] = {
    {STR_LIT(ICON_FA_ARROW_POINTER), STR_LIT("Select Tool")},
    {STR_LIT(ICON_FA_HAND), STR_LIT("Pan Tool")},
    {STR_LIT(ICON_FA_PEN), STR_LIT("Draw Tool")},
    {STR_LIT(ICON_FA_RULER), STR_LIT("Measure Tool")},
    {STR_LIT(ICON_FA_MAGNIFYING_GLASS), STR_LIT("Zoom Tool")},
};

void
toolbar_component_render(s32                    *active_tool_index,
                         f32                     emphasis,
                         const UI_Theme_Palette &palette)
{
    constexpr s32 tool_count = (s32)ARRAY_COUNT(TOOLBAR_ITEMS);
    emphasis = CLAMP(emphasis, 0.0f, 1.0f);
    const f32 slot_size       = TOOLBAR_SLOT_SIZE;
    const f32 slot_gap        = TOOLBAR_SLOT_GAP;
    const f32 container_pad   = TOOLBAR_CONTAINER_PAD;

    if (!active_tool_index)
    {
        return;
    }

    if (*active_tool_index < 0 || *active_tool_index >= tool_count)
    {
        *active_tool_index = 0;
    }

    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (!window)
    {
        return;
    }

    const ImGuiStyle &style = ImGui::GetStyle();
    const f32 rounding       = style.FrameRounding;
    const f32 active_outline_thickness =
        (style.FrameBorderSize > 0.0f) ? (style.FrameBorderSize + 0.8f) : 1.8f;

    const f32 inactive_slot_width = slot_size;
    const f32 active_slot_width   = slot_size;

    f32 strip_width = 0.0f;
    for (u32 i = 0; i < ARRAY_COUNT(TOOLBAR_ITEMS); ++i)
    {
        strip_width += ((s32)i == *active_tool_index) ? active_slot_width
                                                      : inactive_slot_width;
    }
    strip_width += slot_gap * (f32)(ARRAY_COUNT(TOOLBAR_ITEMS) - 1);

    const f32 container_width  = container_pad * 2.0f + strip_width;
    const f32 container_height = container_pad * 2.0f + slot_size;

    ImVec2 container_pos = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(container_width, container_height));

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(container_pos,
                             ImVec2(container_pos.x + container_width,
                                    container_pos.y + container_height),
                             ui::with_alpha(palette.window_bg, 0.45f + 0.18f * emphasis),
                             rounding);

    f32 active_target_offset = container_pad;
    for (s32 i = 0; i < *active_tool_index; ++i)
    {
        active_target_offset += inactive_slot_width + slot_gap;
    }

    ImGuiStorage *storage = &window->StateStorage;
    f32 *active_offset =
        storage->GetFloatRef(ImGui::GetID("toolbar_mode_like_active_offset_x"),
                             active_target_offset);
    *active_offset = ui::anim::exp_decay_to(*active_offset,
                                            active_target_offset,
                                            28.0f,
                                            ImGui::GetIO().DeltaTime);

    const ImVec2 active_min =
        ImVec2(container_pos.x + *active_offset, container_pos.y + container_pad);
    const ImVec2 active_max =
        ImVec2(active_min.x + active_slot_width, active_min.y + slot_size);
    draw_list->AddRectFilled(active_min, active_max, palette.tab_active, rounding);
    draw_list->AddRect(active_min,
                       active_max,
                       ui::with_alpha(palette.accent, 0.70f),
                       rounding,
                       0,
                       active_outline_thickness);

    ImGui::PushID("toolbar_mode_like_selector");
    f32 slot_x = container_pos.x + container_pad;
    for (u32 i = 0; i < ARRAY_COUNT(TOOLBAR_ITEMS); ++i)
    {
        const Toolbar_Tool_Item &item = TOOLBAR_ITEMS[i];
        const b8 active = (s32)i == *active_tool_index;
        const f32 slot_width = active ? active_slot_width : inactive_slot_width;
        const ImVec2 button_min = ImVec2(slot_x, container_pos.y + container_pad);

        ImGui::SetCursorScreenPos(button_min);
        ImGui::PushID((s32)i);
        if (ImGui::InvisibleButton("##toolbar_slot", ImVec2(slot_width, slot_size)))
        {
            *active_tool_index = (s32)i;
        }

        const b8 hovered =
            ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
        if (hovered && !active)
        {
            draw_list->AddRectFilled(button_min,
                                     ImVec2(button_min.x + slot_width,
                                            button_min.y + slot_size),
                                     ui::with_alpha(palette.text_darker,
                                                    0.32f + 0.12f * emphasis),
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
        const ImVec2 icon_pos =
            ImVec2(button_min.x + (slot_size - icon_size.x) * 0.5f,
                   button_min.y + (slot_size - icon_size.y) * 0.5f);
        draw_list->AddText(icon_pos,
                           active ? palette.text_brighter : palette.text,
                           C_STR(item.icon));

        ImGui::PopID();
        slot_x += slot_width + slot_gap;
    }
    ImGui::PopID();
}
