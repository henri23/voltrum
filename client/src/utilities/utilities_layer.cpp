#include "utilities_layer.hpp"

#include "components/command_palette_component.hpp"
#include "components/toolbar_component.hpp"
#include "core/thread_context.hpp"
#include "global_client_state.hpp"

#include <core/frame_context.hpp>
#include <imgui.h>
#include <math/math.hpp>
#include <renderer/renderer_frontend.hpp>
#include <ui/ui.hpp>
#include <ui/ui_themes.hpp>
#include <ui/ui_widgets.hpp>

INTERNAL_FUNC f32
ease_in_out_cubic(f32 t)
{
    t = CLAMP(t, 0.0f, 1.0f);
    if (t < 0.5f)
    {
        return 4.0f * t * t * t;
    }

    const f32 p = (-2.0f * t) + 2.0f;
    return 1.0f - ((p * p * p) * 0.5f);
}

INTERNAL_FUNC u32
lerp_color_u32(u32 from, u32 to, f32 t)
{
    t        = CLAMP(t, 0.0f, 1.0f);
    ImVec4 a = ImGui::ColorConvertU32ToFloat4(from);
    ImVec4 b = ImGui::ColorConvertU32ToFloat4(to);
    ImVec4 c = ImVec4(a.x + (b.x - a.x) * t,
                      a.y + (b.y - a.y) * t,
                      a.z + (b.z - a.z) * t,
                      a.w + (b.w - a.w) * t);
    return ImGui::ColorConvertFloat4ToU32(c);
}

INTERNAL_FUNC UI_Theme_Palette
lerp_palette(const UI_Theme_Palette &from, const UI_Theme_Palette &to, f32 t)
{
    static_assert(sizeof(UI_Theme_Palette) % sizeof(u32) == 0,
                  "UI_Theme_Palette must be u32-packed");

    UI_Theme_Palette result      = from;
    constexpr u32    color_count = sizeof(UI_Theme_Palette) / sizeof(u32);

    const u32 *from_colors = (const u32 *)&from;
    const u32 *to_colors   = (const u32 *)&to;
    u32       *out_colors  = (u32 *)&result;

    for (u32 i = 0; i < color_count; ++i)
    {
        out_colors[i] = lerp_color_u32(from_colors[i], to_colors[i], t);
    }

    return result;
}

INTERNAL_FUNC void
apply_renderer_theme(const UI_Theme_Palette &palette)
{
    ImVec4 clear = ImGui::ColorConvertU32ToFloat4(palette.clear_color);
    renderer_set_viewport_clear_color({clear.x, clear.y, clear.z, clear.w});

    ImVec4 muted = ImGui::ColorConvertU32ToFloat4(palette.muted);
    renderer_set_grid_color({muted.x, muted.y, muted.z, 0.7f});
}

INTERNAL_FUNC void
begin_theme_transition(Global_Client_State *global_state, UI_Theme next_theme)
{
    UI_Theme_Palette next_palette = {};
    ui_themes_copy_palette(next_theme, &next_palette);

    global_state->target_theme           = next_theme;
    global_state->theme_transition_from  = global_state->theme_palette;
    global_state->theme_transition_to    = next_palette;
    global_state->theme_transition_t     = 0.0f;
    global_state->is_theme_transitioning = true;
    global_state->request_theme_change   = false;
}

INTERNAL_FUNC void
update_theme_state(Global_Client_State *global_state, f32 delta_time)
{
    constexpr f32 THEME_TRANSITION_DURATION_SECONDS = 0.85f;

    if (!global_state)
    {
        return;
    }

    if (global_state->request_theme_change)
    {
        if (global_state->requested_theme != global_state->target_theme ||
            !global_state->is_theme_transitioning)
        {
            begin_theme_transition(global_state, global_state->requested_theme);
        }
        else
        {
            global_state->request_theme_change = false;
        }
    }

    if (global_state->is_theme_transitioning)
    {
        const f32 duration = THEME_TRANSITION_DURATION_SECONDS;

        global_state->theme_transition_t += delta_time / duration;
        global_state->theme_transition_t =
            CLAMP(global_state->theme_transition_t, 0.0f, 1.0f);

        const f32 eased = ease_in_out_cubic(global_state->theme_transition_t);
        global_state->theme_palette =
            lerp_palette(global_state->theme_transition_from,
                         global_state->theme_transition_to,
                         eased);

        if (global_state->theme_transition_t >= 1.0f)
        {
            global_state->theme_palette = global_state->theme_transition_to;
            global_state->is_theme_transitioning = false;
        }
    }

    ui_set_theme_palette(&global_state->theme_palette);
    apply_renderer_theme(global_state->theme_palette);
}

INTERNAL_FUNC void
on_theme_command_execute(void *global_state, void *user_data)
{
    if (!global_state || !user_data)
    {
        return;
    }

    auto    *g_state              = (Global_Client_State *)global_state;
    UI_Theme theme                = *(UI_Theme *)user_data;
    g_state->requested_theme      = theme;
    g_state->request_theme_change = true;
}

INTERNAL_FUNC String
resolve_theme_command_description(Arena *arena,
                                  void  *global_state,
                                  void  *user_data,
                                  String base_description)
{
    if (!arena || !global_state || !user_data)
    {
        return base_description;
    }

    auto    *g_state          = (Global_Client_State *)global_state;
    UI_Theme theme            = *(UI_Theme *)user_data;
    const b8 is_current_theme = theme == g_state->target_theme;

    if (is_current_theme && g_state->is_theme_transitioning)
    {
        return str_fmt(arena, "%s  -  Applying...", C_STR(base_description));
    }

    if (is_current_theme)
    {
        return str_fmt(arena, "%s  -  Current", C_STR(base_description));
    }

    return base_description;
}

INTERNAL_FUNC void
register_command_palette_entries(Command_Palette_State *command_palette_state)
{
    if (!command_palette_state)
    {
        return;
    }

    static UI_Theme registered_themes[(u32)UI_Theme::MAX_COUNT] = {};

    static const String theme_selector_section_id =
        STR_LIT("section.theme_selector");

    command_palette_clear_registry(command_palette_state);

    Command_Palette_Command_Definition section = {};
    section.id                                 = theme_selector_section_id;
    section.parent_id                          = str_zero();
    section.label                              = STR_LIT("Theme Selector");
    section.description = STR_LIT("Browse and apply the built-in themes");
    section.keywords    = STR_LIT("themes colors style appearance");
    section.on_execute  = nullptr;
    section.resolve_description = nullptr;
    section.user_data           = nullptr;
    section.close_on_execute    = false;
    command_palette_register(command_palette_state, &section);

    Scratch_Arena scratch = scratch_begin(nullptr, 0);
    for (u32 i = 0; i < (u32)UI_Theme::MAX_COUNT; ++i)
    {
        UI_Theme theme               = (UI_Theme)i;
        registered_themes[i]         = theme;
        String            theme_name = ui_themes_get_name(theme);
        UI_Theme_Metadata meta       = ui_themes_get_metadata(theme);

        Command_Palette_Command_Definition command = {};
        command.id                  = str_fmt(scratch.arena, "theme.%u", i);
        command.parent_id           = theme_selector_section_id;
        command.label               = theme_name;
        command.description         = meta.description;
        command.keywords            = meta.keywords;
        command.on_execute          = on_theme_command_execute;
        command.resolve_description = resolve_theme_command_description;
        command.user_data           = &registered_themes[i];
        command.close_on_execute    = true;
        command_palette_register(command_palette_state, &command);
    }
    scratch_end(scratch);

    command_palette_reset_state(command_palette_state);
}

void
utilities_layer_on_attach(void *state_ptr)
{
    auto *state                         = (Utilities_Layer_State *)state_ptr;
    state->toolbar_position_initialized = false;
    state->toolbar_pos_x                = 0.0f;
    state->toolbar_pos_y                = 0.0f;
    state->toolbar_emphasis             = 0.0f;
    state->active_tool_index            = 0;

    register_command_palette_entries(state->command_palette_state);
}

void
utilities_layer_on_detach(void *state_ptr)
{
    auto *state = (Utilities_Layer_State *)state_ptr;
    command_palette_clear_registry(state ? state->command_palette_state
                                         : nullptr);
}

b8
utilities_layer_on_update(void                 *state_ptr,
                          void                 *global_state,
                          struct Frame_Context *ctx)
{
    (void)state_ptr;

    auto     *g_state = (Global_Client_State *)global_state;
    const f32 dt      = ctx ? ctx->delta_t : (1.0f / 60.0f);
    update_theme_state(g_state, dt);

    return true;
}

b8
utilities_layer_on_render(void                 *state_ptr,
                          void                 *global_state,
                          struct Frame_Context *ctx)
{
    auto     *state   = (Utilities_Layer_State *)state_ptr;
    auto     *g_state = (Global_Client_State *)global_state;
    const f32 dt      = ctx ? ctx->delta_t : (1.0f / 60.0f);

    UI_Theme_Palette palette = {};
    if (g_state)
    {
        palette = g_state->theme_palette;
    }
    else
    {
        ui_get_theme_palette(&palette);
    }

    b8 position_seeded_this_frame = false;
    if (state->toolbar_position_initialized)
    {
        ImGui::SetNextWindowPos(
            ImVec2(state->toolbar_pos_x, state->toolbar_pos_y),
            ImGuiCond_Always);
    }
    else if (g_state && g_state->viewport_bounds_valid)
    {
        state->toolbar_pos_x = g_state->viewport_bounds_x +
                               (g_state->viewport_bounds_width * 0.5f) - 112.0f;
        state->toolbar_pos_y = g_state->viewport_bounds_y + 10.0f;
        ImGui::SetNextWindowPos(
            ImVec2(state->toolbar_pos_x, state->toolbar_pos_y),
            ImGuiCond_Always);
        position_seeded_this_frame = true;
    }

    ImVec4 text_col = ImGui::ColorConvertU32ToFloat4(palette.text);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;

    ImVec4 window_bg_col = ImGui::ColorConvertU32ToFloat4(palette.window_bg);
    ImVec4 border_col    = ImGui::ColorConvertU32ToFloat4(palette.selection);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 7.0f));
    ImGui::PushStyleColor(
        ImGuiCol_WindowBg,
        ImVec4(window_bg_col.x, window_bg_col.y, window_bg_col.z, 0.95f));
    ImGui::PushStyleColor(
        ImGuiCol_Border,
        ImVec4(border_col.x, border_col.y, border_col.z, 0.92f));

    if (ImGui::Begin("##UtilitiesToolbar", nullptr, flags))
    {
        ImDrawList *draw_list = ImGui::GetWindowDrawList();

        const ImVec2 handle_size = ImVec2(16.0f, TOOLBAR_CONTAINER_HEIGHT);
        ImGui::PushID("toolbar_drag_handle");
        ImGui::InvisibleButton("##drag", handle_size);
        const b8 handle_hovered = ImGui::IsItemHovered();
        const b8 handle_active  = ImGui::IsItemActive();
        if (handle_hovered || handle_active)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        ImVec2 handle_min    = ImGui::GetItemRectMin();
        f32    dot_x_spacing = 6.0f;
        f32    dot_y_spacing = 7.0f;
        f32    start_x       = handle_min.x + 4.0f;
        f32    start_y = handle_min.y + (handle_size.y - dot_y_spacing * 2.0f) * 0.5f;
        u32    dot_col =
            ImGui::GetColorU32(ImVec4(text_col.x,
                                      text_col.y,
                                      text_col.z,
                                      handle_hovered ? 0.90f : 0.55f));
        for (s32 row = 0; row < 3; ++row)
        {
            for (s32 col = 0; col < 2; ++col)
            {
                draw_list->AddCircleFilled(
                    ImVec2(start_x + (f32)col * dot_x_spacing,
                           start_y + (f32)row * dot_y_spacing),
                    1.6f,
                    dot_col);
            }
        }

        if (handle_active &&
            ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f))
        {
            ImVec2 mouse_delta = ImGui::GetIO().MouseDelta;
            state->toolbar_pos_x += mouse_delta.x;
            state->toolbar_pos_y += mouse_delta.y;
            ImGui::SetWindowPos(
                ImVec2(state->toolbar_pos_x, state->toolbar_pos_y),
                ImGuiCond_Always);
            state->toolbar_position_initialized = true;
        }
        ImGui::PopID();

        ImGui::SameLine(0.0f, 8.0f);
        toolbar_component_render(&state->active_tool_index,
                                 state->toolbar_emphasis,
                                 palette);

        b8 hovered =
            ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
        f32 target_emphasis = hovered ? 1.0f : 0.24f;
        state->toolbar_emphasis += (target_emphasis - state->toolbar_emphasis) *
                                   CLAMP(dt * 10.0f, 0.0f, 1.0f);
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);

    if (position_seeded_this_frame)
    {
        state->toolbar_position_initialized = true;
    }

    command_palette_component_render(state->command_palette_state,
                                     g_state,
                                     palette,
                                     dt);

    if (g_state && g_state->is_theme_transitioning &&
        g_state->viewport_bounds_valid)
    {
        const f32 pulse =
            1.0f - math_abs_value((g_state->theme_transition_t * 2.0f) - 1.0f);
        ImVec4 tint      = ImGui::ColorConvertU32ToFloat4(palette.highlight);
        ImU32  pulse_col = ImGui::GetColorU32(
            ImVec4(tint.x, tint.y, tint.z, 0.03f + (0.05f * pulse)));

        ImDrawList *bg = ImGui::GetBackgroundDrawList(ImGui::GetMainViewport());
        bg->AddRectFilled(
            ImVec2(g_state->viewport_bounds_x, g_state->viewport_bounds_y),
            ImVec2(g_state->viewport_bounds_x + g_state->viewport_bounds_width,
                   g_state->viewport_bounds_y +
                       g_state->viewport_bounds_height),
            pulse_col,
            10.0f);
    }

    return true;
}

UI_Layer
create_utilities_layer(Utilities_Layer_State *state)
{
    UI_Layer layer  = {};
    layer.on_attach = utilities_layer_on_attach;
    layer.on_detach = utilities_layer_on_detach;
    layer.on_update = utilities_layer_on_update;
    layer.on_render = utilities_layer_on_render;
    layer.state     = state;
    return layer;
}
