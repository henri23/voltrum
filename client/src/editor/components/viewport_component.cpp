#include "viewport_component.hpp"

#include "editor/editor_layer.hpp"
#include "global_client_state.hpp"
#include "input/input.hpp"
#include "input/input_codes.hpp"
#include "core/thread_context.hpp"

#include <core/frame_context.hpp>
#include <core/logger.hpp>
#include <events/events.hpp>
#include <imgui.h>
#include <math/math.hpp>
#include <renderer/renderer_frontend.hpp>
#include <ui/icons.hpp>
#include <ui/ui_themes.hpp>
#include <ui/ui_widgets.hpp>
#include <utils/string.hpp>

constexpr f32 VIEWPORT_MM_PER_MIL = 0.0254f;
constexpr f32 VIEWPORT_MILS_PER_MM = 39.37007874015748f;
constexpr f32 VIEWPORT_MIN_GRID_SPACING_MM = 0.000001f;

internal_var const String VIEWPORT_CONTEXT_MENU_ID =
    STR_LIT("viewport_context_menu");
internal_var const String VIEWPORT_CONTEXT_MENU_ANIM_ID =
    STR_LIT("viewport_ctx_menu_anim");
internal_var const String VIEWPORT_CONTEXT_MENU_GRID_ANIM_ID =
    STR_LIT("viewport_ctx_menu_grid_submenu_anim");
internal_var const String VIEWPORT_CONTEXT_MENU_GRID_CLOSE_TIMER_ID =
    STR_LIT("viewport_ctx_menu_grid_close_timer");
internal_var const String VIEWPORT_CONTEXT_MENU_GRID_POPUP_ID =
    STR_LIT("viewport_context_menu_grid_popup");
internal_var const String VIEWPORT_CONTEXT_MENU_GRID_LABEL = STR_LIT("Grid Size");
internal_var const String VIEWPORT_CONTEXT_MENU_NEW_SHAPE_LABEL =
    STR_LIT("New Shape (Placeholder)");
internal_var const String VIEWPORT_CONTEXT_MENU_CURSOR_SETTINGS_LABEL =
    STR_LIT("Cursor Settings (Placeholder)");

struct Grid_Size_Preset
{
    String label;
    f32    spacing_mm;
};

internal_var const Grid_Size_Preset GRID_SIZE_PRESETS[] = {
    {STR_LIT("1 mil (0.0254 mm)"), 1.0f * VIEWPORT_MM_PER_MIL},
    {STR_LIT("2 mil (0.0508 mm)"), 2.0f * VIEWPORT_MM_PER_MIL},
    {STR_LIT("5 mil (0.1270 mm)"), 5.0f * VIEWPORT_MM_PER_MIL},
    {STR_LIT("10 mil (0.2540 mm)"), 10.0f * VIEWPORT_MM_PER_MIL},
    {STR_LIT("20 mil (0.5080 mm)"), 20.0f * VIEWPORT_MM_PER_MIL},
    {STR_LIT("50 mil (1.2700 mm)"), 50.0f * VIEWPORT_MM_PER_MIL},
    {STR_LIT("100 mil (2.5400 mm)"), 100.0f * VIEWPORT_MM_PER_MIL},
    {STR_LIT("0.10 mm (3.937 mil)"), 0.10f},
    {STR_LIT("0.25 mm (9.843 mil)"), 0.25f},
    {STR_LIT("0.50 mm (19.685 mil)"), 0.50f},
    {STR_LIT("1.00 mm (39.370 mil)"), 1.00f},
    {STR_LIT("2.00 mm (78.740 mil)"), 2.00f}};

INTERNAL_FUNC void viewport_component_update_matrices(Editor_Layer_State *state);
INTERNAL_FUNC void viewport_component_update_cursor_world(
    Editor_Layer_State *state);
INTERNAL_FUNC b8   viewport_component_render_cursor_panel(
    Editor_Layer_State *state,
    const UI_Theme_Palette &palette);
INTERNAL_FUNC b8 viewport_component_is_mouse_inside_viewport_image(
    const Editor_Layer_State *state,
    ImVec2                    mouse);
INTERNAL_FUNC b8   viewport_component_spacing_matches(f32 a, f32 b);
INTERNAL_FUNC void viewport_component_apply_grid_spacing(Editor_Layer_State *state,
                                                         f32                 spacing_mm);
INTERNAL_FUNC void viewport_component_render_context_menu(
    Editor_Layer_State *state,
    b8                  cursor_panel_hovered);

void
viewport_component_on_attach(Editor_Layer_State *state)
{
    // Initialize 2D camera
    state->camera.position    = {0.0f, 0.0f};
    state->camera.zoom        = 50.0f; // pixels per world unit
    state->camera.target_zoom = 50.0f;
    state->camera.dirty       = true;

    state->viewport_focused    = false;
    state->viewport_hovered    = false;
    state->cursor_world_valid  = false;
    state->cursor_world_position = {0.0f, 0.0f};
    state->viewport_image_pos  = {0.0f, 0.0f};
    state->viewport_image_size = {0.0f, 0.0f};
    state->grid_spacing        = 1.0f; // mm

    u32 width  = 0;
    u32 height = 0;
    renderer_get_viewport_size(&width, &height);
    state->viewport_size      = {(f32)width, (f32)height};
    state->last_viewport_size = state->viewport_size;

    renderer_set_grid_spacing(state->grid_spacing);
    viewport_component_update_matrices(state);
}

void
viewport_component_on_update(Editor_Layer_State *state, Frame_Context *ctx)
{
    // Keyboard panning
    if (state->viewport_hovered)
    {
        f32 pan_speed = 300.0f / state->camera.zoom; // world units per second
        vec2 velocity = {0.0f, 0.0f};

        if (input_is_key_pressed(Key_Code::W) ||
            input_is_key_pressed(Key_Code::UP))
        {
            velocity.y += 1.0f;
        }
        if (input_is_key_pressed(Key_Code::S) ||
            input_is_key_pressed(Key_Code::DOWN))
        {
            velocity.y -= 1.0f;
        }
        if (input_is_key_pressed(Key_Code::A) ||
            input_is_key_pressed(Key_Code::LEFT))
        {
            velocity.x -= 1.0f;
        }
        if (input_is_key_pressed(Key_Code::D) ||
            input_is_key_pressed(Key_Code::RIGHT))
        {
            velocity.x += 1.0f;
        }

        if (velocity.x != 0.0f || velocity.y != 0.0f)
        {
            f32 len = math_sqrt(velocity.x * velocity.x +
                                velocity.y * velocity.y);
            velocity.x /= len;
            velocity.y /= len;

            state->camera.position.x += velocity.x * pan_speed * ctx->delta_t;
            state->camera.position.y += velocity.y * pan_speed * ctx->delta_t;
            state->camera.dirty = true;
        }
    }

    // Smooth zoom animation
    if (state->camera.zoom != state->camera.target_zoom)
    {
        f32 speed = 10.0f;
        f32 t     = speed * ctx->delta_t;
        if (t > 1.0f)
        {
            t = 1.0f;
        }

        state->camera.zoom +=
            (state->camera.target_zoom - state->camera.zoom) * t;

        f32 ratio = state->camera.zoom / state->camera.target_zoom;
        if (ratio > 0.999f && ratio < 1.001f)
        {
            state->camera.zoom = state->camera.target_zoom;
        }

        state->camera.dirty = true;
    }

    if (state->camera.dirty)
    {
        viewport_component_update_matrices(state);
        state->camera.dirty = false;
    }
}

void
viewport_component_on_render(Editor_Layer_State  *state,
                             Global_Client_State *global_state,
                             f32                  delta_time)
{
    (void)delta_time;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(ICON_FA_EXPAND " Viewport");
    ImGui::PopStyleVar();

    state->viewport_focused = ImGui::IsWindowFocused();
    state->viewport_hovered = ImGui::IsWindowHovered();

    ImVec2 content_size  = ImGui::GetContentRegionAvail();
    state->viewport_size = {content_size.x, content_size.y};

    if (state->viewport_size.x != state->last_viewport_size.x ||
        state->viewport_size.y != state->last_viewport_size.y)
    {
        u32 width =
            (u32)(state->viewport_size.x < 1.0f ? 1.0f : state->viewport_size.x);
        u32 height =
            (u32)(state->viewport_size.y < 1.0f ? 1.0f : state->viewport_size.y);

        CLIENT_DEBUG("Viewport window resized to %ux%u", width, height);

        renderer_resize_viewport(width, height);
        state->last_viewport_size = state->viewport_size;
        state->camera.dirty       = true;
    }

    // Render viewport and present its image in this window.
    renderer_render_viewport();
    ImVec2 image_pos = ImGui::GetCursorScreenPos();

    ImGui::Image(renderer_get_rendered_viewport(),
                 content_size,
                 ImVec2(0, 0),
                 ImVec2(1, 1));

    state->viewport_image_pos  = {image_pos.x, image_pos.y};
    state->viewport_image_size = {content_size.x, content_size.y};

    if (global_state)
    {
        global_state->viewport_bounds_valid = true;
        global_state->viewport_bounds_x     = image_pos.x;
        global_state->viewport_bounds_y     = image_pos.y;
        global_state->viewport_bounds_width = content_size.x;
        global_state->viewport_bounds_height = content_size.y;
    }

    viewport_component_update_cursor_world(state);
    b8 cursor_panel_hovered = false;
    if (global_state)
    {
        cursor_panel_hovered =
            viewport_component_render_cursor_panel(state,
                                                   global_state->theme_palette);
    }

    viewport_component_render_context_menu(state, cursor_panel_hovered);

    ImGui::End();
}

b8
viewport_component_on_mouse_wheel(Editor_Layer_State *state, const Event *event)
{
    if (!state || !state->viewport_hovered)
    {
        return false;
    }

    f32 delta = event->mouse_wheel.delta_y;
    if (delta == 0.0f)
    {
        return false;
    }

    f32 zoom_factor = 1.15f;
    if (delta > 0.0f)
    {
        state->camera.target_zoom *= zoom_factor;
    }
    else
    {
        state->camera.target_zoom /= zoom_factor;
    }

    state->camera.target_zoom =
        CLAMP(state->camera.target_zoom, 0.01f, 1e8f);
    return true;
}

b8
viewport_component_on_mouse_moved(Editor_Layer_State *state, const Event *event)
{
    if (!state || !state->viewport_hovered)
    {
        return false;
    }

    // Middle mouse button drag for panning.
    if (!input_is_mouse_button_pressed(Mouse_Button::MIDDLE))
    {
        return false;
    }

    f32 dx = (f32)event->mouse_move.delta_x;
    f32 dy = (f32)event->mouse_move.delta_y;

    // Convert screen pixels to world units (invert Y for screen coordinates).
    state->camera.position.x -= dx / state->camera.zoom;
    state->camera.position.y += dy / state->camera.zoom;
    state->camera.dirty = true;

    return true;
}

INTERNAL_FUNC void
viewport_component_update_matrices(Editor_Layer_State *state)
{
    f32 vp_w = state->viewport_size.x;
    f32 vp_h = state->viewport_size.y;

    if (vp_w < 1.0f)
    {
        vp_w = 1.0f;
    }
    if (vp_h < 1.0f)
    {
        vp_h = 1.0f;
    }

    f32 half_w = (vp_w / state->camera.zoom) * 0.5f;
    f32 half_h = (vp_h / state->camera.zoom) * 0.5f;

    mat4 projection = mat4_project_orthographic(
        -half_w,
        half_w,
        -half_h,
        half_h,
        -1.0f,
        1.0f);

    mat4 view = mat4_translation(
        {-state->camera.position.x,
         -state->camera.position.y,
         0.0f});

    renderer_set_projection(projection);
    renderer_set_view(view);
}

INTERNAL_FUNC void
viewport_component_update_cursor_world(Editor_Layer_State *state)
{
    ImVec2 mouse = ImGui::GetIO().MousePos;

    f32 left   = state->viewport_image_pos.x;
    f32 top    = state->viewport_image_pos.y;
    f32 width  = state->viewport_image_size.x;
    f32 height = state->viewport_image_size.y;

    if (width < 1.0f || height < 1.0f)
    {
        state->cursor_world_valid = false;
        return;
    }

    b8 inside = mouse.x >= left && mouse.x <= (left + width) &&
                mouse.y >= top && mouse.y <= (top + height);

    if (!inside)
    {
        state->cursor_world_valid = false;
        return;
    }

    f32 local_x = mouse.x - left;
    f32 local_y = mouse.y - top;

    f32 world_x = ((local_x - width * 0.5f) / state->camera.zoom) +
                  state->camera.position.x;
    f32 world_y = (((height * 0.5f) - local_y) / state->camera.zoom) +
                  state->camera.position.y;

    state->cursor_world_position = {world_x, world_y};
    state->cursor_world_valid    = true;
}

INTERNAL_FUNC b8
viewport_component_render_cursor_panel(Editor_Layer_State  *state,
                                       const UI_Theme_Palette &palette)
{
    const f32 pad = 12.0f;
    const f32 base_width = 296.0f;

    ImVec2 pos = ImVec2(state->viewport_image_pos.x + pad,
                        state->viewport_image_pos.y + pad);
    const f32 overlay_width = base_width;

    Scratch_Arena scratch = scratch_begin(nullptr, 0);

    String x_label = state->cursor_world_valid
                         ? str_fmt(scratch.arena,
                                   "%.3f",
                                   state->cursor_world_position.x)
                         : STR_LIT("--");
    String y_label = state->cursor_world_valid
                         ? str_fmt(scratch.arena,
                                   "%.3f",
                                   state->cursor_world_position.y)
                         : STR_LIT("--");
    String zoom_label = str_fmt(scratch.arena, "%.2f px/mm", state->camera.zoom);
    String grid_label = str_fmt(scratch.arena, "%.6f mm", state->grid_spacing);

    ImGui::SetCursorScreenPos(pos);
    ui::glass_content_options glass_options =
        ui::make_glass_content_options(overlay_width);
    glass_options.emphasis                  = 1.0f;
    glass_options.rounding                  = 10.0f;
    glass_options.padding                   = ImVec2(12.0f, 10.0f);
    UI_BEGIN_GLASS_CONTENT(cursor_world_glass_scope, palette, glass_options);

    ImGui::TextUnformatted(ICON_FA_LOCATION_DOT " Cursor World");
    ImGui::TextDisabled("Viewport-relative diagnostics");
    ImGui::Separator();

    const f32 value_col_x = 118.0f;
    ImGui::TextUnformatted("X");
    ImGui::SameLine(value_col_x);
    ImGui::Text("%s", C_STR(x_label));

    ImGui::TextUnformatted("Y");
    ImGui::SameLine(value_col_x);
    ImGui::Text("%s", C_STR(y_label));

    ImGui::TextUnformatted("Zoom");
    ImGui::SameLine(value_col_x);
    ImGui::Text("%s", C_STR(zoom_label));

    ImGui::TextUnformatted("Grid Size");
    ImGui::SameLine(value_col_x);
    ImGui::Text("%s", C_STR(grid_label));

    UI_END_GLASS_CONTENT(cursor_world_glass_scope);

    ImVec2 panel_min = ImGui::GetItemRectMin();
    ImVec2 panel_max = ImGui::GetItemRectMax();
    ImVec2 mouse     = ImGui::GetIO().MousePos;
    b8 is_hovered = mouse.x >= panel_min.x && mouse.x <= panel_max.x &&
           mouse.y >= panel_min.y && mouse.y <= panel_max.y;

    scratch_end(scratch);
    return is_hovered;
}

INTERNAL_FUNC b8
viewport_component_is_mouse_inside_viewport_image(
    const Editor_Layer_State *state,
    ImVec2                    mouse)
{
    if (!state)
    {
        return false;
    }

    f32 left   = state->viewport_image_pos.x;
    f32 top    = state->viewport_image_pos.y;
    f32 width  = state->viewport_image_size.x;
    f32 height = state->viewport_image_size.y;

    if (width < 1.0f || height < 1.0f)
    {
        return false;
    }

    return mouse.x >= left && mouse.x <= (left + width) && mouse.y >= top &&
           mouse.y <= (top + height);
}

INTERNAL_FUNC b8
viewport_component_spacing_matches(f32 a, f32 b)
{
    return math_abs_value(a - b) <= 0.00001f;
}

INTERNAL_FUNC void
viewport_component_apply_grid_spacing(Editor_Layer_State *state, f32 spacing_mm)
{
    if (!state)
    {
        return;
    }

    if (spacing_mm <= 0.0f)
    {
        spacing_mm = VIEWPORT_MIN_GRID_SPACING_MM;
    }

    state->grid_spacing = spacing_mm;
    renderer_set_grid_spacing(state->grid_spacing);
}

INTERNAL_FUNC void
viewport_component_render_context_menu(Editor_Layer_State *state,
                                       b8                  cursor_panel_hovered)
{
    if (!state)
    {
        return;
    }

    ImVec2 mouse = ImGui::GetIO().MousePos;
    b8 mouse_inside_image =
        viewport_component_is_mouse_inside_viewport_image(state, mouse);

    if (mouse_inside_image && !cursor_panel_hovered &&
        ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    {
        ImGui::OpenPopup(C_STR(VIEWPORT_CONTEXT_MENU_ID));
    }

    ImGuiStorage *storage    = ImGui::GetStateStorage();
    f32           delta_time = ImGui::GetIO().DeltaTime;
    b8            menu_open  = ImGui::IsPopupOpen(C_STR(VIEWPORT_CONTEXT_MENU_ID),
                                       ImGuiPopupFlags_None);
    f32 menu_alpha = ui::anim::track_popup_alpha(
        storage,
        ImGui::GetID(C_STR(VIEWPORT_CONTEXT_MENU_ANIM_ID)),
        menu_open,
        delta_time);

    ImVec4 menu_bg_col = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.9f * menu_alpha);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, menu_bg_col);
    if (!ImGui::BeginPopup(C_STR(VIEWPORT_CONTEXT_MENU_ID)))
    {
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        return;
    }

    ui::menu_item(C_STR(VIEWPORT_CONTEXT_MENU_NEW_SHAPE_LABEL),
                  nullptr,
                  nullptr,
                  true,
                  false);
    ui::menu_item(C_STR(VIEWPORT_CONTEXT_MENU_CURSOR_SETTINGS_LABEL),
                  nullptr,
                  nullptr,
                  true,
                  false);
    ImGui::Separator();

    b8 grid_trigger_pressed = ui::menu_item(C_STR(VIEWPORT_CONTEXT_MENU_GRID_LABEL),
                                            ICON_FA_CHEVRON_RIGHT,
                                            nullptr,
                                            true,
                                            false);
    b8     grid_trigger_hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
    ImVec2 grid_item_min        = ImGui::GetItemRectMin();
    ImVec2 grid_item_max        = ImGui::GetItemRectMax();

    b8 submenu_popup_open = ImGui::IsPopupOpen(C_STR(VIEWPORT_CONTEXT_MENU_GRID_POPUP_ID),
                                               ImGuiPopupFlags_None);
    b8 submenu_target_open =
        submenu_popup_open || grid_trigger_hovered || grid_trigger_pressed;
    f32 submenu_alpha = ui::anim::track_popup_alpha(
        storage,
        ImGui::GetID(C_STR(VIEWPORT_CONTEXT_MENU_GRID_ANIM_ID)),
        submenu_target_open,
        delta_time);

    if (grid_trigger_hovered || grid_trigger_pressed || submenu_popup_open)
    {
        ImGui::SetNextWindowPos(ImVec2(grid_item_max.x + 2.0f, grid_item_min.y));
    }
    if (grid_trigger_hovered || grid_trigger_pressed)
    {
        ImGui::OpenPopup(C_STR(VIEWPORT_CONTEXT_MENU_GRID_POPUP_ID));
    }

    ImGuiID submenu_close_timer_id =
        ImGui::GetID(C_STR(VIEWPORT_CONTEXT_MENU_GRID_CLOSE_TIMER_ID));
    f32 *submenu_close_timer = storage->GetFloatRef(submenu_close_timer_id, 0.0f);
    b8   close_context_menu  = false;

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.9f * submenu_alpha);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, menu_bg_col);
    if (ImGui::BeginPopup(C_STR(VIEWPORT_CONTEXT_MENU_GRID_POPUP_ID)))
    {
        b8 submenu_hovered =
            ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);

        for (u32 i = 0; i < ARRAY_COUNT(GRID_SIZE_PRESETS); ++i)
        {
            const Grid_Size_Preset &preset = GRID_SIZE_PRESETS[i];
            b8 selected = viewport_component_spacing_matches(
                state->grid_spacing,
                preset.spacing_mm);

            if (ui::menu_item(C_STR(preset.label), nullptr, &selected))
            {
                viewport_component_apply_grid_spacing(state,
                                                      preset.spacing_mm);
                close_context_menu = true;
            }
        }

        if (grid_trigger_hovered || submenu_hovered)
        {
            *submenu_close_timer = 0.0f;
        }
        else
        {
            *submenu_close_timer += delta_time;
            if (*submenu_close_timer >= 0.12f)
            {
                ImGui::CloseCurrentPopup();
                *submenu_close_timer = 0.0f;
            }
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    if (close_context_menu)
    {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}
