#pragma once

#include "defines.hpp"
#include "ui_themes.hpp"

#include <cfloat>
#include <imgui.h>
#include <imgui_internal.h>
#include <utils/string.hpp>

#include "ui_animation.hpp"

// Custom menu widgets with rounded corners

namespace ui
{

    inline ImU32
    with_alpha(ImU32 color, f32 alpha_scale)
    {
        ImVec4 c = ImGui::ColorConvertU32ToFloat4(color);
        c.w      = ImClamp(c.w * alpha_scale, 0.0f, 1.0f);
        return ImGui::ColorConvertFloat4ToU32(c);
    }

    inline ImU32
    blend_color(ImU32 a, ImU32 b, f32 t)
    {
        ImVec4 ca = ImGui::ColorConvertU32ToFloat4(a);
        ImVec4 cb = ImGui::ColorConvertU32ToFloat4(b);
        ImVec4 c  = ImLerp(ca, cb, ImClamp(t, 0.0f, 1.0f));
        return ImGui::ColorConvertFloat4ToU32(c);
    }

    inline f32
    snap_scalar(f32 value)
    {
        return IM_ROUND(value);
    }

    inline ImVec2
    snap_pos(const ImVec2 &value)
    {
        return ImVec2(snap_scalar(value.x), snap_scalar(value.y));
    }

    inline void
    add_crisp_rounded_border(ImDrawList   *draw_list,
                             const ImVec2 &min_pos,
                             const ImVec2 &max_pos,
                             ImU32         color,
                             f32           rounding,
                             f32           thickness = 1.0f)
    {
        if (!draw_list)
        {
            return;
        }

        ImVec2 stroke_min = ImVec2(min_pos.x + 0.5f, min_pos.y + 0.5f);
        ImVec2 stroke_max = ImVec2(max_pos.x - 0.5f, max_pos.y - 0.5f);
        if (stroke_max.x <= stroke_min.x || stroke_max.y <= stroke_min.y)
        {
            return;
        }

        draw_list->AddRect(stroke_min,
                           stroke_max,
                           color,
                           ImMax(0.0f, rounding - 0.5f),
                           ImDrawFlags_RoundCornersAll,
                           thickness);
    }

    inline void
    draw_glass_container(ImDrawList             *draw_list,
                         const ImVec2           &min_pos,
                         const ImVec2           &max_pos,
                         const UI_Theme_Palette &palette,
                         f32                     emphasis = 1.0f,
                         f32                     rounding = -1.0f)
    {
        if (!draw_list)
        {
            return;
        }

        emphasis = ImClamp(emphasis, 0.0f, 1.0f);

        const ImVec2 min = snap_pos(ImVec2(min_pos.x + 2.0f, min_pos.y + 2.0f));
        const ImVec2 max = snap_pos(ImVec2(max_pos.x - 2.0f, max_pos.y - 2.0f));
        if (max.x <= min.x || max.y <= min.y)
        {
            return;
        }

        const f32 radius = snap_scalar(
            rounding >= 0.0f ? rounding
                             : ImMax(8.0f, ImGui::GetStyle().WindowRounding));
        const f32 rounded = ImMax(0.0f, snap_scalar(radius - 1.0f));

        const ImU32 base_fill =
            with_alpha(palette.window_bg, 0.72f + (0.10f * emphasis));
        const ImU32 frost_fill =
            with_alpha(palette.background_popup, 0.09f + (0.08f * emphasis));
        const ImU32 outer_border =
            with_alpha(palette.group_header, 0.64f + (0.20f * emphasis));
        const ImU32 inner_border =
            IM_COL32(255, 255, 255, (int)(16.0f + (18.0f * emphasis)));

        // base glass volume
        draw_list->AddRectFilled(min, max, base_fill, rounded);

        // inner frost layers
        const s32 frost_layer_count = 8;
        for (s32 layer_index = 0; layer_index < frost_layer_count;
             ++layer_index)
        {
            const f32 t = (f32)(layer_index + 1) / (f32)(frost_layer_count + 1);
            const f32 inset  = snap_scalar(t * 9.0f);
            ImVec2 layer_min = snap_pos(ImVec2(min.x + inset, min.y + inset));
            ImVec2 layer_max = snap_pos(ImVec2(max.x - inset, max.y - inset));
            if (layer_max.x <= layer_min.x || layer_max.y <= layer_min.y)
            {
                continue;
            }

            const f32 layer_rounded =
                ImMax(0.0f, snap_scalar(rounded - (inset * 0.55f)));
            draw_list->AddRectFilled(
                layer_min,
                layer_max,
                with_alpha(frost_fill, 0.35f + ((1.0f - t) * 0.65f)),
                layer_rounded);
        }

        // empty outline rings for glass refraction feel
        const s32 ring_count = 4;
        for (s32 ring_index = 0; ring_index < ring_count; ++ring_index)
        {
            const f32 t        = (f32)(ring_index + 1) / (f32)(ring_count + 1);
            const f32 inset    = snap_scalar(t * 4.0f);
            ImVec2    ring_min = snap_pos(ImVec2(min.x + inset, min.y + inset));
            ImVec2    ring_max = snap_pos(ImVec2(max.x - inset, max.y - inset));
            if (ring_max.x <= ring_min.x || ring_max.y <= ring_min.y)
            {
                continue;
            }

            const f32 ring_rounded = rounded - (inset * 0.6f);
            if (rounded > 0.0f && ring_rounded < 1.0f)
            {
                continue;
            }

            const s32 alpha = (s32)((14.0f + (16.0f * emphasis)) * (1.0f - t));
            add_crisp_rounded_border(draw_list,
                                     ring_min,
                                     ring_max,
                                     IM_COL32(255, 255, 255, alpha),
                                     ImMax(0.0f, snap_scalar(ring_rounded)),
                                     1.0f);
        }

        // vignette
        const f32 min_size             = ImMin(max.x - min.x, max.y - min.y);
        const f32 vignette_span        = ImClamp(min_size * 0.12f, 8.0f, 20.0f);
        const s32 vignette_layer_count = 10;
        for (s32 layer_index = 0; layer_index < vignette_layer_count;
             ++layer_index)
        {
            const f32 t = (f32)(layer_index + 1) / (f32)vignette_layer_count;
            const f32 inset  = snap_scalar(t * vignette_span);
            ImVec2 layer_min = snap_pos(ImVec2(min.x + inset, min.y + inset));
            ImVec2 layer_max = snap_pos(ImVec2(max.x - inset, max.y - inset));
            if (layer_max.x <= layer_min.x || layer_max.y <= layer_min.y)
            {
                continue;
            }

            const f32 layer_rounded = snap_scalar(rounded - (inset * 0.5f));
            if (rounded > 0.0f && layer_rounded < 1.0f)
            {
                continue;
            }

            const s32 alpha =
                (s32)((30.0f + (14.0f * emphasis)) * (1.0f - t) * (1.0f - t));
            add_crisp_rounded_border(draw_list,
                                     layer_min,
                                     layer_max,
                                     IM_COL32(0, 0, 0, alpha),
                                     ImMax(0.0f, layer_rounded),
                                     1.0f);
        }

        // borders
        add_crisp_rounded_border(draw_list,
                                 min,
                                 max,
                                 outer_border,
                                 rounded,
                                 1.0f);
        add_crisp_rounded_border(draw_list,
                                 ImVec2(min.x + 1.0f, min.y + 1.0f),
                                 ImVec2(max.x - 1.0f, max.y - 1.0f),
                                 inner_border,
                                 ImMax(0.0f, rounded - 1.0f),
                                 1.0f);
    }

    struct glass_content_options
    {
        f32    emphasis;
        f32    rounding;
        f32    width;
        ImVec2 padding;
    };

    struct glass_content_scope
    {
        b8                      active;
        b8                      pushed_width_constraints;
        b8                      pushed_content_clip;
        const UI_Theme_Palette *palette;
        glass_content_options   options;
        ImDrawList             *draw_list;
        ImVec2                  container_min;
    };

    inline glass_content_options
    make_glass_content_options(f32 width)
    {
        glass_content_options options = {};
        options.emphasis              = 1.0f;
        options.rounding              = -1.0f;
        options.width                 = ImMax(width, 1.0f);
        options.padding               = ImVec2(12.0f, 10.0f);
        return options;
    }

    inline glass_content_scope
    begin_glass_content(const UI_Theme_Palette      &palette,
                        const glass_content_options &options)
    {
        glass_content_scope scope      = {};
        scope.active                   = true;
        scope.pushed_width_constraints = false;
        scope.pushed_content_clip      = false;
        scope.palette                  = &palette;
        scope.options                  = options;
        scope.options.width            = ImMax(scope.options.width, 1.0f);
        scope.draw_list                = ImGui::GetWindowDrawList();

        scope.draw_list->ChannelsSplit(2);
        scope.draw_list->ChannelsSetCurrent(1);

        ImVec2 start_pos    = snap_pos(ImGui::GetCursorScreenPos());
        scope.container_min = start_pos;
        const f32 content_width =
            ImMax(1.0f, scope.options.width - (scope.options.padding.x * 2.0f));
        const ImVec2 content_min =
            snap_pos(ImVec2(start_pos.x + scope.options.padding.x,
                            start_pos.y + scope.options.padding.y));
        const ImVec2 content_clip_max =
            ImVec2(content_min.x + content_width, FLT_MAX);

        ImGui::PushItemWidth(content_width);
        ImGui::PushTextWrapPos(start_pos.x + scope.options.padding.x +
                               content_width);
        ImGui::PushClipRect(content_min, content_clip_max, true);
        scope.pushed_width_constraints = true;
        scope.pushed_content_clip      = true;

        ImGui::SetCursorScreenPos(content_min);
        ImGui::BeginGroup();

        return scope;
    }

    inline void
    end_glass_content(glass_content_scope *scope)
    {
        if (!scope || !scope->active || !scope->palette || !scope->draw_list)
        {
            return;
        }

        ImGui::EndGroup();
        if (scope->pushed_width_constraints)
        {
            ImGui::PopTextWrapPos();
            ImGui::PopItemWidth();
            scope->pushed_width_constraints = false;
        }
        if (scope->pushed_content_clip)
        {
            ImGui::PopClipRect();
            scope->pushed_content_clip = false;
        }

        const ImVec2 content_max = ImGui::GetItemRectMax();

        ImVec2 container_min = snap_pos(scope->container_min);
        ImVec2 container_max =
            snap_pos(ImVec2(container_min.x + scope->options.width,
                            content_max.y + scope->options.padding.y));
        container_max.y =
            ImMax(container_max.y,
                  container_min.y + (scope->options.padding.y * 2.0f) + 1.0f);

        scope->draw_list->ChannelsSetCurrent(0);
        draw_glass_container(scope->draw_list,
                             container_min,
                             container_max,
                             *scope->palette,
                             scope->options.emphasis,
                             scope->options.rounding);
        scope->draw_list->ChannelsSetCurrent(1);
        scope->draw_list->ChannelsMerge();

        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        if (container_max.y > cursor_pos.y)
        {
            ImGui::SetCursorScreenPos(ImVec2(cursor_pos.x, container_max.y));
            // ImGui requires a submitted item after SetCursor* when extending
            // parent boundaries.
            ImGui::Dummy(ImVec2(0.0f, 0.0f));
        }

        scope->active = false;
    }

    struct accent_row_style
    {
        ImU32 bg_idle;
        ImU32 bg_hover;
        ImU32 bg_selected;
        ImU32 border_idle;
        ImU32 border_hover;
        ImU32 border_selected;
        ImU32 text;
        ImU32 text_dim;
        f32   rounding;
    };

    inline accent_row_style
    make_accent_row_style(const UI_Theme_Palette &palette)
    {
        accent_row_style style = {};

        ImU32 base_bg     = with_alpha(palette.background_popup, 0.82f);
        ImU32 hover_tint  = with_alpha(palette.accent, 0.18f);
        ImU32 select_tint = with_alpha(palette.accent, 0.30f);

        style.bg_idle         = base_bg;
        style.bg_hover        = blend_color(base_bg, hover_tint, 0.72f);
        style.bg_selected     = blend_color(base_bg, select_tint, 0.92f);
        style.border_idle     = with_alpha(palette.selection_muted, 0.40f);
        style.border_hover    = with_alpha(palette.accent, 0.68f);
        style.border_selected = with_alpha(palette.accent, 0.95f);
        style.text            = palette.text;
        style.text_dim        = palette.text_darker;
        style.rounding = ImMax(7.0f, ImGui::GetStyle().FrameRounding + 2.0f);

        return style;
    }

    inline bool
    accent_row(const char             *id,
               const char             *title,
               const char             *subtitle,
               const accent_row_style &style,
               bool                    selected = false,
               f32                     height   = 52.0f)
    {
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x, height);
        if (size.x < 1.0f)
            size.x = 1.0f;

        bool    clicked = ImGui::InvisibleButton(id, size);
        bool    hovered = ImGui::IsItemHovered();
        bool    held    = ImGui::IsItemActive();
        ImGuiID item_id = ImGui::GetItemID();

        ImRect      bb        = ImRect(snap_pos(ImGui::GetItemRectMin()),
                           snap_pos(ImGui::GetItemRectMax()));
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        const f32   dt        = ImGui::GetIO().DeltaTime;
        const f32   hover_t   = anim::track_bool(&window->StateStorage,
                                             item_id ^ 0x41AA7C13u,
                                             hovered || held || selected,
                                             dt,
                                             24.0f,
                                             18.0f);

        ImU32 fill = blend_color(style.bg_idle, style.bg_hover, hover_t);
        if (selected)
            fill = blend_color(fill, style.bg_selected, 0.9f);

        ImU32 border =
            blend_color(style.border_idle, style.border_hover, hover_t);
        if (selected)
            border = style.border_selected;

        const f32 rounded = ImMax(0.0f, snap_scalar(style.rounding));
        draw_list->AddRectFilled(bb.Min, bb.Max, fill, rounded);

        ImU32 final_border = border;
        if (hover_t > 0.01f || selected)
        {
            final_border = selected ? style.border_selected
                                    : blend_color(style.border_idle,
                                                  style.border_hover,
                                                  hover_t);
        }
        add_crisp_rounded_border(draw_list,
                                 bb.Min,
                                 bb.Max,
                                 final_border,
                                 rounded,
                                 1.0f);

        const f32 left_pad = 12.0f;
        const f32 top_pad  = 9.0f;
        draw_list->AddText(ImVec2(bb.Min.x + left_pad, bb.Min.y + top_pad),
                           style.text,
                           title);

        if (subtitle && subtitle[0] != '\0')
        {
            ImU32 subtitle_col =
                blend_color(style.text_dim, style.text, hover_t * 0.35f);
            draw_list->AddText(
                ImVec2(bb.Min.x + left_pad, bb.Min.y + top_pad + 19.0f),
                subtitle_col,
                subtitle);
        }

        return clicked;
    }

    struct icon_selector_item
    {
        String icon;
        String label;
        String tooltip;
    };

    struct icon_selector_overrides
    {
        f32 icon_gap                            = -1.0f;
        f32 horizontal_padding                  = -1.0f;
        f32 vertical_padding                    = -1.0f;
        f32 active_text_gap                     = -1.0f;
        f32 active_text_padding                 = -1.0f;
        f32 rounding                            = -1.0f;
        f32 container_bg_alpha                  = -1.0f;
        f32 container_border_alpha              = -1.0f;
        f32 hover_overlay_alpha                 = -1.0f;
        f32 active_outline_alpha                = -1.0f;
        f32 active_outline_thickness            = -1.0f;
        f32 active_anim_sharpness               = -1.0f;
        f32 tooltip_border_alpha                = -1.0f;
        s8  show_active_label                   = -1;
        s8  allow_hovered_when_blocked_by_popup = -1;
    };

    inline void
    tooltip_text(String                  text,
                 const UI_Theme_Palette &palette,
                 f32                     border_alpha = 0.82f)
    {
        if (text.size <= 0)
        {
            return;
        }

        const ImGuiStyle &style = ImGui::GetStyle();

        ImVec4 bg_col =
            ImGui::ColorConvertU32ToFloat4(palette.background_popup);
        bg_col.w = ImClamp(bg_col.w * 0.96f, 0.0f, 1.0f);

        ImVec4 border_col = ImGui::ColorConvertU32ToFloat4(palette.accent);
        border_col.w      = ImClamp(border_col.w * border_alpha, 0.0f, 1.0f);

        ImVec4 text_col = ImGui::ColorConvertU32ToFloat4(palette.text_brighter);
        text_col.w      = ImClamp(text_col.w * 0.98f, 0.0f, 1.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,
                            style.FrameRounding + 2.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,
                            border_alpha > 0.0f ? 1.0f : 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(9.0f, 6.0f));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, bg_col);
        ImGui::PushStyleColor(ImGuiCol_Border, border_col);
        ImGui::PushStyleColor(ImGuiCol_Text, text_col);

        ImGui::BeginTooltip();
        const char *text_begin = text.buff ? (const char *)text.buff : "";
        const char *text_end   = text_begin + (text.buff ? text.size : 0);
        ImGui::TextUnformatted(text_begin, text_end);
        ImGui::EndTooltip();

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(3);
    }

    inline b8
    icon_selector(const char                    *id,
                  const icon_selector_item      *items,
                  s32                            item_count,
                  s32                           *active_index,
                  f32                            height,
                  const UI_Theme_Palette        &palette,
                  const icon_selector_overrides *overrides = nullptr)
    {
        if (!items || item_count <= 0 || !active_index || height <= 0.0f)
        {
            return false;
        }

        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (!window || window->SkipItems)
        {
            return false;
        }

        if (*active_index < 0 || *active_index >= item_count)
        {
            *active_index = 0;
        }

        const ImGuiStyle &style = ImGui::GetStyle();
        const f32         scale = ImMax(0.35f, height / 40.0f);

        const f32 icon_gap = (overrides && overrides->icon_gap >= 0.0f)
                                 ? overrides->icon_gap
                                 : snap_scalar(8.0f * scale);
        const f32 pad_x = (overrides && overrides->horizontal_padding >= 0.0f)
                              ? overrides->horizontal_padding
                              : snap_scalar(2.0f * scale);
        const f32 pad_y = (overrides && overrides->vertical_padding >= 0.0f)
                              ? overrides->vertical_padding
                              : snap_scalar(2.0f * scale);
        const f32 active_text_gap =
            (overrides && overrides->active_text_gap >= 0.0f)
                ? overrides->active_text_gap
                : snap_scalar(8.0f * scale);
        const f32 active_text_pad =
            (overrides && overrides->active_text_padding >= 0.0f)
                ? overrides->active_text_padding
                : snap_scalar(11.0f * scale);
        const f32 rounding = (overrides && overrides->rounding >= 0.0f)
                                 ? overrides->rounding
                                 : ImMax(2.0f, style.FrameRounding * scale);
        const f32 container_bg_alpha =
            (overrides && overrides->container_bg_alpha >= 0.0f)
                ? overrides->container_bg_alpha
                : 0.45f;
        const f32 container_border_alpha =
            (overrides && overrides->container_border_alpha >= 0.0f)
                ? overrides->container_border_alpha
                : 0.0f;
        const f32 hover_overlay_alpha =
            (overrides && overrides->hover_overlay_alpha >= 0.0f)
                ? overrides->hover_overlay_alpha
                : 0.32f;
        const f32 active_outline_alpha =
            (overrides && overrides->active_outline_alpha >= 0.0f)
                ? overrides->active_outline_alpha
                : 0.70f;
        const f32 active_anim_sharpness =
            (overrides && overrides->active_anim_sharpness > 0.0f)
                ? overrides->active_anim_sharpness
                : 28.0f;
        const f32 tooltip_border_alpha =
            (overrides && overrides->tooltip_border_alpha >= 0.0f)
                ? overrides->tooltip_border_alpha
                : 0.82f;
        const f32 active_outline_thickness =
            (overrides && overrides->active_outline_thickness > 0.0f)
                ? overrides->active_outline_thickness
                : ((style.FrameBorderSize > 0.0f)
                       ? (style.FrameBorderSize + 0.8f)
                       : 1.8f);

        const bool allow_hovered_when_blocked_by_popup =
            overrides && overrides->allow_hovered_when_blocked_by_popup > 0;

        bool has_labels = false;
        for (s32 i = 0; i < item_count; ++i)
        {
            if (items[i].label.size > 0)
            {
                has_labels = true;
                break;
            }
        }

        const bool show_active_label =
            (overrides && overrides->show_active_label >= 0)
                ? overrides->show_active_label > 0
                : has_labels;

        const f32 slot_size = ImMax(1.0f, snap_scalar(height - (pad_y * 2.0f)));

        f32 max_label_width = 0.0f;
        if (show_active_label)
        {
            for (s32 i = 0; i < item_count; ++i)
            {
                const char *label_begin =
                    items[i].label.buff ? (const char *)items[i].label.buff
                                        : "";
                const char *label_end =
                    label_begin +
                    (items[i].label.buff ? items[i].label.size : 0);
                const f32 label_width =
                    ImGui::CalcTextSize(label_begin, label_end).x;
                if (label_width > max_label_width)
                {
                    max_label_width = label_width;
                }
            }
        }

        const f32 active_slot_width =
            slot_size + ((show_active_label && max_label_width > 0.0f)
                             ? (active_text_gap + max_label_width +
                                (active_text_pad * 2.0f))
                             : 0.0f);

        f32 strip_width = 0.0f;
        for (s32 i = 0; i < item_count; ++i)
        {
            strip_width += (i == *active_index) ? active_slot_width : slot_size;
        }
        strip_width += icon_gap * (f32)(item_count - 1);

        const f32 container_width  = (pad_x * 2.0f) + strip_width;
        const f32 container_height = (pad_y * 2.0f) + slot_size;

        const char *widget_id = id ? id : "icon_selector";
        ImGui::PushID(widget_id);

        ImVec2 container_pos = ImGui::GetCursorScreenPos();
        ImGui::Dummy(ImVec2(container_width, container_height));

        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        if (container_bg_alpha > 0.0f)
        {
            draw_list->AddRectFilled(
                container_pos,
                ImVec2(container_pos.x + container_width,
                       container_pos.y + container_height),
                with_alpha(palette.window_bg, container_bg_alpha),
                rounding);
        }
        if (container_border_alpha > 0.0f)
        {
            draw_list->AddRect(
                container_pos,
                ImVec2(container_pos.x + container_width,
                       container_pos.y + container_height),
                with_alpha(palette.group_header, container_border_alpha),
                rounding);
        }

        f32 active_target_offset = pad_x;
        for (s32 i = 0; i < *active_index; ++i)
        {
            active_target_offset += slot_size + icon_gap;
        }

        ImGuiStorage *storage = &window->StateStorage;
        f32          *active_offset =
            storage->GetFloatRef(ImGui::GetID("active_offset_x"),
                                 active_target_offset);
        *active_offset = anim::exp_decay_to(*active_offset,
                                            active_target_offset,
                                            active_anim_sharpness,
                                            ImGui::GetIO().DeltaTime);

        const ImVec2 active_min =
            ImVec2(container_pos.x + *active_offset, container_pos.y + pad_y);
        const ImVec2 active_max =
            ImVec2(active_min.x + active_slot_width, active_min.y + slot_size);
        draw_list->AddRectFilled(active_min,
                                 active_max,
                                 palette.tab_active,
                                 rounding);
        draw_list->AddRect(active_min,
                           active_max,
                           with_alpha(palette.accent, active_outline_alpha),
                           rounding,
                           0,
                           active_outline_thickness);

        b8  changed = false;
        f32 slot_x  = container_pos.x + pad_x;
        for (s32 i = 0; i < item_count; ++i)
        {
            const icon_selector_item &item      = items[i];
            const b8                  is_active = i == *active_index;
            const f32    slot_width = is_active ? active_slot_width : slot_size;
            const ImVec2 button_min = ImVec2(slot_x, container_pos.y + pad_y);

            ImGui::SetCursorScreenPos(button_min);
            ImGui::PushID(i);
            if (ImGui::InvisibleButton("##slot", ImVec2(slot_width, slot_size)))
            {
                *active_index = i;
                changed       = true;
            }

            ImGuiHoveredFlags hover_flags = 0;
            if (allow_hovered_when_blocked_by_popup)
            {
                hover_flags |= ImGuiHoveredFlags_AllowWhenBlockedByPopup;
            }
            const b8 hovered = ImGui::IsItemHovered(hover_flags);
            if (hovered && !is_active && hover_overlay_alpha > 0.0f)
            {
                draw_list->AddRectFilled(
                    button_min,
                    ImVec2(button_min.x + slot_width, button_min.y + slot_size),
                    with_alpha(palette.text_darker, hover_overlay_alpha),
                    rounding);
            }

            if (hovered && item.tooltip.size > 0)
            {
                tooltip_text(item.tooltip, palette, tooltip_border_alpha);
            }

            const char *icon_begin =
                item.icon.buff ? (const char *)item.icon.buff : "";
            const char *icon_end =
                icon_begin + (item.icon.buff ? item.icon.size : 0);
            const ImVec2 icon_size = ImGui::CalcTextSize(icon_begin, icon_end);
            if (is_active && show_active_label && item.label.size > 0)
            {
                const ImVec2 icon_pos =
                    ImVec2(button_min.x + active_text_pad,
                           button_min.y + (slot_size - icon_size.y) * 0.5f);
                draw_list->AddText(icon_pos,
                                   palette.text_brighter,
                                   icon_begin,
                                   icon_end);

                const char *label_begin =
                    item.label.buff ? (const char *)item.label.buff : "";
                const char *label_end =
                    label_begin + (item.label.buff ? item.label.size : 0);
                const ImVec2 label_size =
                    ImGui::CalcTextSize(label_begin, label_end);
                const ImVec2 label_pos =
                    ImVec2(icon_pos.x + icon_size.x + active_text_gap,
                           button_min.y + (slot_size - label_size.y) * 0.5f);
                draw_list->AddText(label_pos,
                                   palette.text_brighter,
                                   label_begin,
                                   label_end);
            }
            else
            {
                const ImVec2 icon_pos =
                    ImVec2(button_min.x + (slot_size - icon_size.x) * 0.5f,
                           button_min.y + (slot_size - icon_size.y) * 0.5f);
                draw_list->AddText(icon_pos,
                                   is_active ? palette.text_brighter
                                             : palette.text,
                                   icon_begin,
                                   icon_end);
            }

            ImGui::PopID();
            slot_x += slot_width + icon_gap;
        }

        ImGui::PopID();
        return changed;
    }

    inline bool &
    menu_item_close_requested()
    {
        static bool requested = false;
        return requested;
    }

    // Rounded menu_item with optional active-state checkmark at the end.
    // If is_active is provided, a checkmark is rendered on the right when
    // *is_active is true, and pressing the item auto-toggles the value.
    inline bool
    menu_item(const char *label,
              const char *shortcut          = nullptr,
              b8         *is_active         = nullptr,
              bool        enabled           = true,
              bool        close_on_activate = true)
    {
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext &g          = *GImGui;
        ImGuiStyle   &style      = g.Style;
        ImVec2        pos        = window->DC.CursorPos;
        ImVec2        label_size = ImGui::CalcTextSize(label, nullptr, true);

        ImVec2 shortcut_size =
            shortcut ? ImGui::CalcTextSize(shortcut) : ImVec2(0, 0);
        float shortcut_w = shortcut_size.x > 0
                               ? shortcut_size.x + style.ItemSpacing.x * 2.0f
                               : 0.0f;

        float check_sz = g.FontSize;
        float check_w  = is_active ? check_sz + style.ItemSpacing.x : 0.0f;

        float min_w = window->DC.MenuColumns.TotalWidth + shortcut_w + check_w;
        float avail_w = ImGui::GetContentRegionAvail().x;
        float w       = ImMax(label_size.x + style.FramePadding.x * 2.0f +
                            shortcut_w + check_w,
                        ImMax(min_w, avail_w));

        ImVec2 size(w, label_size.y + style.FramePadding.y * 2.0f);

        ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(size, style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, ImGui::GetID(label)))
            return false;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb,
                                             ImGui::GetID(label),
                                             &hovered,
                                             &held,
                                             ImGuiButtonFlags_PressedOnRelease);

        const f32 hover_t = anim::track_bool(&window->StateStorage,
                                             ImGui::GetID(label) ^ 0x5C31E14Bu,
                                             hovered && enabled,
                                             g.IO.DeltaTime,
                                             30.0f,
                                             18.0f);

        // Render rounded background on hover
        if (hover_t > 0.01f && enabled)
        {
            ImU32 col = ImGui::GetColorU32(held ? ImGuiCol_TabActive
                                                : ImGuiCol_TabHovered);
            col       = with_alpha(col, hover_t);
            window->DrawList->AddRectFilled(bb.Min,
                                            bb.Max,
                                            col,
                                            style.WindowRounding);
        }

        // Render text
        ImVec2 text_pos = ImVec2(bb.Min.x + style.FramePadding.x,
                                 bb.Min.y + style.FramePadding.y);
        ImGui::RenderText(text_pos, label);

        // Render shortcut
        if (shortcut)
        {
            float shortcut_x =
                bb.Max.x - style.FramePadding.x - shortcut_size.x - check_w;
            ImGui::RenderText(ImVec2(shortcut_x, text_pos.y), shortcut);
        }

        // Render checkmark at the right end when is_active is provided
        if (is_active && *is_active)
        {
            ImU32 col     = ImGui::GetColorU32(enabled ? ImGuiCol_Text
                                                   : ImGuiCol_TextDisabled);
            float pad     = ImMax(1.0f, (float)(int)(check_sz / 6.0f));
            float check_x = bb.Max.x - style.FramePadding.x - check_sz;
            float check_y = bb.Min.y + style.FramePadding.y + pad;
            ImGui::RenderCheckMark(window->DrawList,
                                   ImVec2(check_x, check_y),
                                   col,
                                   check_sz - pad * 2.0f);
        }

        // Toggle active state and close popup when activated
        if (pressed && enabled)
        {
            if (is_active)
                *is_active = !(*is_active);
            if (close_on_activate && window->Name &&
                ImStrnicmp(window->Name, "##Menu_", 7) == 0)
            {
                menu_item_close_requested() = true;
            }
            if (close_on_activate)
            {
                ImGui::CloseCurrentPopup();
            }
        }

        return pressed && enabled;
    }

    // Storage for menu hover state
    struct menu_state
    {
        ImGuiID active_menu_id   = 0;
        ImRect  active_menu_rect = {};
        bool    alpha_pushed     = false;
        bool    open_by_click    = false;
    };

    inline menu_state &
    get_menu_state()
    {
        static menu_state state;
        return state;
    }

    // Rounded begin_menu - click-based menu with rounded corners
    inline bool
    begin_menu(const char *label, bool enabled = true)
    {
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext &g     = *GImGui;
        ImGuiStyle   &style = g.Style;
        ImGuiID       id    = window->GetID(label);
        menu_state   &state = get_menu_state();

        ImVec2 pos        = window->DC.CursorPos;
        ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);

        // For menu bar items (horizontal layout)
        if (window->DC.LayoutType == ImGuiLayoutType_Horizontal)
        {
            // Custom padding for menu bar items
            const float padding_x =
                16.0f; // Horizontal padding from text to border
            const float padding_y =
                4.0f; // Vertical padding from text to border

            // Override item spacing to place rectangles adjacent
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

            ImVec2 size(label_size.x + padding_x * 2.0f,
                        label_size.y + padding_y * 2.0f);
            ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
            ImGui::ItemSize(size, padding_y);

            ImGui::PopStyleVar();

            if (!ImGui::ItemAdd(bb, id))
                return false;

            ImVec2 mouse_pos      = ImGui::GetMousePos();
            bool   item_hovered   = bb.Contains(mouse_pos);
            bool   is_active_menu = (state.active_menu_id == id);

            bool pressed = enabled && item_hovered &&
                           ImGui::IsMouseClicked(ImGuiMouseButton_Left);

            if (pressed)
            {
                if (is_active_menu && state.open_by_click)
                {
                    state.active_menu_id = 0;
                    state.open_by_click  = false;
                    is_active_menu       = false;
                }
                else
                {
                    state.active_menu_id   = id;
                    state.active_menu_rect = bb;
                    state.open_by_click    = true;
                    is_active_menu         = true;
                }
                menu_item_close_requested() = false;
            }
            else if (state.open_by_click && item_hovered && enabled &&
                     state.active_menu_id != id)
            {
                state.active_menu_id   = id;
                state.active_menu_rect = bb;
                is_active_menu         = true;
            }

            if (is_active_menu)
            {
                state.active_menu_rect = bb;
            }

            const f32 popup_alpha_t =
                anim::track_popup_alpha(&window->StateStorage,
                                        id ^ 0x32B4A5C7u,
                                        is_active_menu,
                                        g.IO.DeltaTime);

            // Render rounded background (use WindowRounding to match popup
            // corners)
            const f32 hover_t =
                anim::track_bool(&window->StateStorage,
                                 id ^ 0x7D3092C1u,
                                 (is_active_menu || item_hovered) && enabled,
                                 g.IO.DeltaTime,
                                 28.0f,
                                 16.0f);

            if (hover_t > 0.01f)
            {
                ImU32 col = ImGui::GetColorU32(
                    is_active_menu ? ImGuiCol_TabActive : ImGuiCol_TabHovered);
                col = with_alpha(col, hover_t);
                window->DrawList->AddRectFilled(bb.Min,
                                                bb.Max,
                                                col,
                                                style.WindowRounding);
            }

            // Render text
            ImU32 text_col = ImGui::GetColorU32(
                enabled ? ImGuiCol_Text : ImGuiCol_TextDisabled);
            ImVec2 text_pos =
                ImVec2(bb.Min.x + padding_x, bb.Min.y + padding_y);
            ImGui::RenderText(text_pos, label);

            // Show popup window if this is the active menu
            if (is_active_menu)
            {
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

                const f32 popup_alpha = 0.9f * popup_alpha_t;
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, popup_alpha);

                if (ImGui::Begin(window_name, nullptr, flags))
                {
                    ImGuiWindow *popup_window = ImGui::GetCurrentWindow();
                    ImRect       popup_rect(
                        popup_window->Pos,
                        ImVec2(popup_window->Pos.x + popup_window->Size.x,
                               popup_window->Pos.y + popup_window->Size.y));

                    bool in_menu_item =
                        state.active_menu_rect.Contains(mouse_pos);
                    bool in_popup = popup_rect.Contains(mouse_pos);
                    bool clicked_outside =
                        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                        !in_menu_item && !in_popup;

                    if (clicked_outside)
                    {
                        state.active_menu_id        = 0;
                        state.open_by_click         = false;
                        menu_item_close_requested() = false;
                        ImGui::End();
                        ImGui::PopStyleVar();
                        return false;
                    }

                    state.alpha_pushed = true;
                    return true;
                }

                ImGui::PopStyleVar();
            }
        }
        else
        {
            // Vertical layout (submenu) - use standard ImGui for now
            return ImGui::BeginMenu(label, enabled);
        }

        return false;
    }

    inline void
    end_menu()
    {
        menu_state &state = get_menu_state();
        ImGui::End();
        if (state.alpha_pushed)
        {
            ImGui::PopStyleVar();
            state.alpha_pushed = false;
        }
        if (menu_item_close_requested())
        {
            state.active_menu_id        = 0;
            state.open_by_click         = false;
            menu_item_close_requested() = false;
        }
    }

} // namespace ui

#define UI_BEGIN_GLASS_CONTENT(scope_name, palette_value, options_value)       \
    ui::glass_content_scope scope_name =                                       \
        ui::begin_glass_content((palette_value), (options_value))

#define UI_END_GLASS_CONTENT(scope_name) ui::end_glass_content(&(scope_name))
