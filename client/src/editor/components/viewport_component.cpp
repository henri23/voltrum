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
constexpr f32 VIEWPORT_CAMERA_MIN_ZOOM = 0.01f;
constexpr f32 VIEWPORT_CAMERA_MAX_ZOOM = 1e8f;
#ifdef DEBUG_BUILD
constexpr f32 VIEWPORT_DEBUG_CAMERA_ROTATE_SENSITIVITY = 0.16f;
constexpr f32 VIEWPORT_DEBUG_CAMERA_SPEED_BOOST = 3.0f;
constexpr f32 VIEWPORT_DEBUG_CAMERA_PAN_FACTOR = 0.001f;
constexpr f32 VIEWPORT_DEBUG_CAMERA_MOVE_SPEED_MIN = 0.1f;
constexpr f32 VIEWPORT_DEBUG_CAMERA_MOVE_SPEED_MAX = 2000.0f;
constexpr f32 VIEWPORT_DEBUG_CAMERA_DOLLY_FACTOR = 0.15f;
constexpr f32 VIEWPORT_DEBUG_CAMERA_SPEED_STEP = 1.15f;
constexpr f32 VIEWPORT_DEBUG_CAMERA_ORBIT_PIVOT_FALLBACK_DISTANCE = 5.0f;
constexpr f32 VIEWPORT_DEBUG_CAMERA_MIN_ORBIT_RADIUS = 0.05f;
constexpr f32 VIEWPORT_DEBUG_CAMERA_MAX_ORBIT_RADIUS = 400.0f;
constexpr f32 VIEWPORT_DEBUG_CAMERA_ORBIT_PLANE_Z = 0.0f;
constexpr f32 VIEWPORT_DEBUG_CAMERA_FOV_RADIANS = 60.0f * math::DEG_RAD_FACTOR;
constexpr f32 VIEWPORT_DEBUG_CAMERA_MAX_WHEEL_DELTA = 3.0f;
constexpr f32 VIEWPORT_DEBUG_CAMERA_WHEEL_DEADZONE = 0.01f;
constexpr f32 VIEWPORT_DEBUG_CAMERA_MAX_MOUSE_DELTA = 120.0f;
constexpr f32 VIEWPORT_DEBUG_CAMERA_MAX_POSITION_ABS = 10000.0f;
constexpr f32 VIEWPORT_DEBUG_CAMERA_MAX_PICK_DISTANCE = 800.0f;
constexpr f32 VIEWPORT_DEBUG_CAMERA_PICK_MIN_DIR_Z = 0.02f;
// Keep clip range tighter in debug free-camera to reduce depth precision issues
// when inspecting closely stacked planes.
constexpr f32 VIEWPORT_DEBUG_CAMERA_NEAR = 0.10f;
constexpr f32 VIEWPORT_DEBUG_CAMERA_FAR = 500.0f;
#endif

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
#ifdef DEBUG_BUILD
internal_var const String VIEWPORT_CONTEXT_MENU_FREE_CAMERA_LABEL =
    STR_LIT("Debug Free Camera (3D)");
#endif

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
#ifdef DEBUG_BUILD
INTERNAL_FUNC f32 viewport_component_sanitize_wheel_delta(f32 raw_delta);
INTERNAL_FUNC b8  viewport_component_debug_camera_position_valid(vec3 p);
INTERNAL_FUNC vec3 viewport_component_debug_camera_clamped_position(vec3 p);
INTERNAL_FUNC vec3 viewport_component_debug_camera_forward(
    const Editor_Layer_State *state);
INTERNAL_FUNC vec3 viewport_component_debug_camera_right(
    const Editor_Layer_State *state);
INTERNAL_FUNC vec3 viewport_component_debug_camera_up(
    const Editor_Layer_State *state);
INTERNAL_FUNC mat4 viewport_component_debug_camera_projection(
    const Editor_Layer_State *state);
INTERNAL_FUNC mat4 viewport_component_debug_camera_view(
    const Editor_Layer_State *state);
INTERNAL_FUNC b8 viewport_component_debug_pick_orbit_pivot(
    const Editor_Layer_State *state,
    vec3                     *out_pivot);
#endif

void
viewport_component_on_attach(Editor_Layer_State *state)
{
    // Initialize 2D camera
    state->camera.position    = {0.0f, 0.0f};
    state->camera.zoom        = 50.0f; // pixels per world unit
    state->camera.target_zoom = 50.0f;
    state->camera.zoom_anchor_active = false;
    state->camera.zoom_anchor_viewport_local = {0.0f, 0.0f};
    state->camera.zoom_anchor_world = {0.0f, 0.0f};
    state->camera.dirty       = true;
#ifdef DEBUG_BUILD
    state->debug_camera.enabled       = false;
    state->debug_camera.position      = {0.0f, 0.0f, 6.0f};
    state->debug_camera.yaw_degrees   = -90.0f;
    state->debug_camera.pitch_degrees = -30.0f;
    state->debug_camera.move_speed    = 5.0f;
    state->debug_camera.orbit_active  = false;
    state->debug_camera.orbit_pivot   = {0.0f, 0.0f, 0.0f};
#endif

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
#ifdef DEBUG_BUILD
    if (state->debug_camera.enabled)
    {
        b8 lmb_pressed =
            input_is_mouse_button_pressed(Mouse_Button::LEFT);
        b8 rmb_pressed =
            input_is_mouse_button_pressed(Mouse_Button::RIGHT);

        if (!lmb_pressed)
        {
            state->debug_camera.orbit_active = false;
        }

        if (state->viewport_hovered && rmb_pressed)
        {
            vec3 velocity = vec3_zero();
            vec3 forward  = viewport_component_debug_camera_forward(state);
            vec3 right    = viewport_component_debug_camera_right(state);
            vec3 up       = viewport_component_debug_camera_up(state);

            if (input_is_key_pressed(Key_Code::W) ||
                input_is_key_pressed(Key_Code::UP))
            {
                velocity = velocity + forward;
            }
            if (input_is_key_pressed(Key_Code::S) ||
                input_is_key_pressed(Key_Code::DOWN))
            {
                velocity = velocity - forward;
            }
            if (input_is_key_pressed(Key_Code::A) ||
                input_is_key_pressed(Key_Code::LEFT))
            {
                velocity = velocity - right;
            }
            if (input_is_key_pressed(Key_Code::D) ||
                input_is_key_pressed(Key_Code::RIGHT))
            {
                velocity = velocity + right;
            }
            if (input_is_key_pressed(Key_Code::Q))
            {
                velocity = velocity - up;
            }
            if (input_is_key_pressed(Key_Code::E))
            {
                velocity = velocity + up;
            }

            if (vec3_length_squared(velocity) > 0.0f)
            {
                velocity = vec3_norm_copy(velocity);
                f32 move_speed = state->debug_camera.move_speed;
                if (input_is_key_pressed(Key_Code::LSHIFT) ||
                    input_is_key_pressed(Key_Code::RSHIFT))
                {
                    move_speed *= VIEWPORT_DEBUG_CAMERA_SPEED_BOOST;
                }

                state->debug_camera.position =
                    state->debug_camera.position +
                    vec3_scale(velocity, move_speed * ctx->delta_t);
                state->debug_camera.position =
                    viewport_component_debug_camera_clamped_position(
                        state->debug_camera.position);
                state->camera.dirty = true;
            }
        }

        state->camera.zoom_anchor_active = false;

        if (state->camera.dirty)
        {
            viewport_component_update_matrices(state);
            state->camera.dirty = false;
        }

        return;
    }
#endif

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
            state->camera.zoom_anchor_active = false;
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
            state->camera.zoom_anchor_active = false;
        }

        if (state->camera.zoom_anchor_active)
        {
            f32 width  = state->viewport_size.x < 1.0f ? 1.0f : state->viewport_size.x;
            f32 height = state->viewport_size.y < 1.0f ? 1.0f : state->viewport_size.y;

            f32 local_x = CLAMP(state->camera.zoom_anchor_viewport_local.x,
                                0.0f,
                                width);
            f32 local_y = CLAMP(state->camera.zoom_anchor_viewport_local.y,
                                0.0f,
                                height);

            state->camera.position.x =
                state->camera.zoom_anchor_world.x -
                ((local_x - width * 0.5f) / state->camera.zoom);

            state->camera.position.y =
                state->camera.zoom_anchor_world.y -
                (((height * 0.5f) - local_y) / state->camera.zoom);
        }

        state->camera.dirty = true;
    }
    else
    {
        state->camera.zoom_anchor_active = false;
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

#ifdef DEBUG_BUILD
    if (state->debug_camera.enabled)
    {
        delta = viewport_component_sanitize_wheel_delta(delta);
        if (delta == 0.0f)
        {
            return false;
        }

        if (input_is_mouse_button_pressed(Mouse_Button::RIGHT))
        {
            // Fractional wheel values from trackpads should adjust speed
            // proportionally, instead of applying full discrete wheel steps.
            f32 speed_scale =
                1.0f +
                ((VIEWPORT_DEBUG_CAMERA_SPEED_STEP - 1.0f) * delta);
            speed_scale = MAX(speed_scale, 0.01f);
            state->debug_camera.move_speed *= speed_scale;

            state->debug_camera.move_speed =
                CLAMP(state->debug_camera.move_speed,
                      VIEWPORT_DEBUG_CAMERA_MOVE_SPEED_MIN,
                      VIEWPORT_DEBUG_CAMERA_MOVE_SPEED_MAX);
        }
        else
        {
            vec3 forward = viewport_component_debug_camera_forward(state);
            f32  dolly_distance =
                delta *
                MAX(state->debug_camera.move_speed * VIEWPORT_DEBUG_CAMERA_DOLLY_FACTOR,
                    VIEWPORT_DEBUG_CAMERA_MIN_ORBIT_RADIUS);
            state->debug_camera.position =
                state->debug_camera.position + vec3_scale(forward, dolly_distance);
            state->debug_camera.position =
                viewport_component_debug_camera_clamped_position(
                    state->debug_camera.position);
        }

        state->camera.dirty = true;
        return true;
    }
#endif

    ImVec2 mouse = ImGui::GetIO().MousePos;
    if (!viewport_component_is_mouse_inside_viewport_image(state, mouse))
    {
        return false;
    }

    f32 width  = state->viewport_image_size.x;
    f32 height = state->viewport_image_size.y;
    if (width < 1.0f || height < 1.0f)
    {
        return false;
    }

    f32 local_x = mouse.x - state->viewport_image_pos.x;
    f32 local_y = mouse.y - state->viewport_image_pos.y;

    state->camera.zoom_anchor_viewport_local = {local_x, local_y};
    state->camera.zoom_anchor_world = {
        ((local_x - width * 0.5f) / state->camera.zoom) + state->camera.position.x,
        (((height * 0.5f) - local_y) / state->camera.zoom) + state->camera.position.y};
    state->camera.zoom_anchor_active = true;

    f32 zoom_factor = 1.15f;
    if (delta > 0.0f)
    {
        state->camera.target_zoom *= zoom_factor;
    }
    else
    {
        state->camera.target_zoom /= zoom_factor;
    }

    state->camera.target_zoom = CLAMP(state->camera.target_zoom,
                                      VIEWPORT_CAMERA_MIN_ZOOM,
                                      VIEWPORT_CAMERA_MAX_ZOOM);
    if (state->camera.target_zoom == state->camera.zoom)
    {
        state->camera.zoom_anchor_active = false;
    }

    return true;
}

b8
viewport_component_on_mouse_moved(Editor_Layer_State *state, const Event *event)
{
    if (!state || !state->viewport_hovered)
    {
        return false;
    }

    f32 dx = (f32)event->mouse_move.delta_x;
    f32 dy = (f32)event->mouse_move.delta_y;

#ifdef DEBUG_BUILD
    if (state->debug_camera.enabled)
    {
        // Clamp pathological motion spikes (common on some touchpads) so
        // free-camera rotation/orbit stays stable.
        dx = CLAMP(dx,
                   -VIEWPORT_DEBUG_CAMERA_MAX_MOUSE_DELTA,
                   VIEWPORT_DEBUG_CAMERA_MAX_MOUSE_DELTA);
        dy = CLAMP(dy,
                   -VIEWPORT_DEBUG_CAMERA_MAX_MOUSE_DELTA,
                   VIEWPORT_DEBUG_CAMERA_MAX_MOUSE_DELTA);

        b8 lmb_pressed = input_is_mouse_button_pressed(Mouse_Button::LEFT);
        b8 mmb_pressed = input_is_mouse_button_pressed(Mouse_Button::MIDDLE);
        b8 rmb_pressed = input_is_mouse_button_pressed(Mouse_Button::RIGHT);

        if (rmb_pressed)
        {
            state->debug_camera.orbit_active = false;
            state->debug_camera.yaw_degrees +=
                dx * VIEWPORT_DEBUG_CAMERA_ROTATE_SENSITIVITY;
            state->debug_camera.pitch_degrees -=
                dy * VIEWPORT_DEBUG_CAMERA_ROTATE_SENSITIVITY;
            state->debug_camera.pitch_degrees =
                CLAMP(state->debug_camera.pitch_degrees, -89.0f, 89.0f);
            state->camera.dirty = true;
            return true;
        }

        if (lmb_pressed)
        {
            if (!state->debug_camera.orbit_active)
            {
                if (!viewport_component_debug_pick_orbit_pivot(
                        state,
                        &state->debug_camera.orbit_pivot))
                {
                    vec3 forward = viewport_component_debug_camera_forward(state);
                    state->debug_camera.orbit_pivot =
                        state->debug_camera.position +
                        vec3_scale(forward,
                                   VIEWPORT_DEBUG_CAMERA_ORBIT_PIVOT_FALLBACK_DISTANCE);
                }
                state->debug_camera.orbit_active = true;
            }

            vec3 to_pivot =
                state->debug_camera.orbit_pivot - state->debug_camera.position;
            f32 orbit_radius = vec3_length(to_pivot);
            orbit_radius =
                CLAMP(orbit_radius,
                      VIEWPORT_DEBUG_CAMERA_MIN_ORBIT_RADIUS,
                      VIEWPORT_DEBUG_CAMERA_MAX_ORBIT_RADIUS);

            state->debug_camera.yaw_degrees +=
                dx * VIEWPORT_DEBUG_CAMERA_ROTATE_SENSITIVITY;
            state->debug_camera.pitch_degrees -=
                dy * VIEWPORT_DEBUG_CAMERA_ROTATE_SENSITIVITY;
            state->debug_camera.pitch_degrees =
                CLAMP(state->debug_camera.pitch_degrees, -89.0f, 89.0f);

            vec3 forward = viewport_component_debug_camera_forward(state);
            state->debug_camera.position =
                state->debug_camera.orbit_pivot -
                vec3_scale(forward, orbit_radius);
            state->debug_camera.position =
                viewport_component_debug_camera_clamped_position(
                    state->debug_camera.position);
            state->camera.dirty = true;
            return true;
        }

        state->debug_camera.orbit_active = false;
        if (mmb_pressed)
        {
            vec3 right = viewport_component_debug_camera_right(state);
            vec3 up    = viewport_component_debug_camera_up(state);
            f32 pan_scale = VIEWPORT_DEBUG_CAMERA_PAN_FACTOR *
                            MAX(state->debug_camera.move_speed, 1.0f);
            vec3 pan_delta =
                vec3_scale(right, -dx * pan_scale) +
                vec3_scale(up, dy * pan_scale);

            state->debug_camera.position =
                state->debug_camera.position + pan_delta;
            state->debug_camera.position =
                viewport_component_debug_camera_clamped_position(
                    state->debug_camera.position);
            state->debug_camera.orbit_pivot =
                state->debug_camera.orbit_pivot + pan_delta;
            state->debug_camera.orbit_pivot =
                viewport_component_debug_camera_clamped_position(
                    state->debug_camera.orbit_pivot);
            state->camera.dirty = true;
            return true;
        }

        return false;
    }
#endif

    // Middle mouse button drag for panning.
    if (!input_is_mouse_button_pressed(Mouse_Button::MIDDLE))
    {
        return false;
    }

    // Convert screen pixels to world units (invert Y for screen coordinates).
    state->camera.position.x -= dx / state->camera.zoom;
    state->camera.position.y += dy / state->camera.zoom;
    state->camera.zoom_anchor_active = false;
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

#ifdef DEBUG_BUILD
    if (state->debug_camera.enabled)
    {
        renderer_set_projection(viewport_component_debug_camera_projection(state));
        renderer_set_view(viewport_component_debug_camera_view(state));
        return;
    }
#endif

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
#ifdef DEBUG_BUILD
    if (state->debug_camera.enabled)
    {
        state->cursor_world_valid = false;
        return;
    }
#endif

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
                         ? string_fmt(scratch.arena,
                                   "%.3f",
                                   state->cursor_world_position.x)
                         : STR_LIT("--");
    String y_label = state->cursor_world_valid
                         ? string_fmt(scratch.arena,
                                   "%.3f",
                                   state->cursor_world_position.y)
                         : STR_LIT("--");
    String zoom_label = string_fmt(scratch.arena, "%.2f px/mm", state->camera.zoom);
    String grid_label = string_fmt(scratch.arena, "%.6f mm", state->grid_spacing);
#ifdef DEBUG_BUILD
    String camera_mode_label = state->debug_camera.enabled
                                   ? STR_LIT("Debug 3D")
                                   : STR_LIT("Production 2D");
    String camera_pos_label = string_fmt(scratch.arena,
                                      "%.2f, %.2f, %.2f",
                                      state->debug_camera.position.x,
                                      state->debug_camera.position.y,
                                      state->debug_camera.position.z);
    String camera_angles_label = string_fmt(scratch.arena,
                                         "Yaw %.1f, Pitch %.1f",
                                         state->debug_camera.yaw_degrees,
                                         state->debug_camera.pitch_degrees);
    if (state->debug_camera.enabled)
    {
        zoom_label = STR_LIT("N/A (Perspective)");
        x_label    = STR_LIT("--");
        y_label    = STR_LIT("--");
    }
#endif

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
    ImGui::TextUnformatted(x_label.buff ? (const char *)x_label.buff : "",
                           x_label.buff ? (const char *)x_label.buff + x_label.size
                                       : "");

    ImGui::TextUnformatted("Y");
    ImGui::SameLine(value_col_x);
    ImGui::TextUnformatted(y_label.buff ? (const char *)y_label.buff : "",
                           y_label.buff ? (const char *)y_label.buff + y_label.size
                                       : "");

    ImGui::TextUnformatted("Zoom");
    ImGui::SameLine(value_col_x);
    ImGui::TextUnformatted(
        zoom_label.buff ? (const char *)zoom_label.buff : "",
        zoom_label.buff ? (const char *)zoom_label.buff + zoom_label.size : "");

    ImGui::TextUnformatted("Grid Size");
    ImGui::SameLine(value_col_x);
    ImGui::TextUnformatted(
        grid_label.buff ? (const char *)grid_label.buff : "",
        grid_label.buff ? (const char *)grid_label.buff + grid_label.size : "");

#ifdef DEBUG_BUILD
    ImGui::TextUnformatted("Mode");
    ImGui::SameLine(value_col_x);
    ImGui::TextUnformatted(camera_mode_label.buff
                               ? (const char *)camera_mode_label.buff
                               : "",
                           camera_mode_label.buff
                               ? (const char *)camera_mode_label.buff +
                                     camera_mode_label.size
                               : "");

    if (state->debug_camera.enabled)
    {
        ImGui::TextUnformatted("Cam Pos");
        ImGui::SameLine(value_col_x);
        ImGui::TextUnformatted(camera_pos_label.buff
                                   ? (const char *)camera_pos_label.buff
                                   : "",
                               camera_pos_label.buff
                                   ? (const char *)camera_pos_label.buff +
                                         camera_pos_label.size
                                   : "");

        ImGui::TextUnformatted("Cam Rot");
        ImGui::SameLine(value_col_x);
        ImGui::TextUnformatted(camera_angles_label.buff
                                   ? (const char *)camera_angles_label.buff
                                   : "",
                               camera_angles_label.buff
                                   ? (const char *)camera_angles_label.buff +
                                         camera_angles_label.size
                                   : "");

        ImGui::Separator();
        ImGui::TextDisabled(
            "RMB look+WASDQE | LMB orbit | MMB pan | Wheel dolly | RMB+Wheel speed");
    }
#endif

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
        ImGui::OpenPopup((const char *)VIEWPORT_CONTEXT_MENU_ID.buff);
    }

    ImGuiStorage *storage    = ImGui::GetStateStorage();
    f32           delta_time = ImGui::GetIO().DeltaTime;
    b8 menu_open = ImGui::IsPopupOpen((const char *)VIEWPORT_CONTEXT_MENU_ID.buff,
                                      ImGuiPopupFlags_None);
    f32 menu_alpha = ui::anim::track_popup_alpha(
        storage,
        ImGui::GetID((const char *)VIEWPORT_CONTEXT_MENU_ANIM_ID.buff),
        menu_open,
        delta_time);

    ImVec4 menu_bg_col = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.9f * menu_alpha);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, menu_bg_col);
    if (!ImGui::BeginPopup((const char *)VIEWPORT_CONTEXT_MENU_ID.buff))
    {
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        return;
    }

    ui::menu_item((const char *)VIEWPORT_CONTEXT_MENU_NEW_SHAPE_LABEL.buff,
                  nullptr,
                  nullptr,
                  true,
                  false);
    ui::menu_item((const char *)VIEWPORT_CONTEXT_MENU_CURSOR_SETTINGS_LABEL.buff,
                  nullptr,
                  nullptr,
                  true,
                  false);
#ifdef DEBUG_BUILD
    b8 free_camera_enabled = state->debug_camera.enabled;
    if (ui::menu_item((const char *)VIEWPORT_CONTEXT_MENU_FREE_CAMERA_LABEL.buff,
                      nullptr,
                      &free_camera_enabled,
                      true,
                      false))
    {
        state->debug_camera.enabled = free_camera_enabled;
        state->debug_camera.orbit_active = false;
        state->camera.zoom_anchor_active = false;
        state->camera.dirty = true;
    }
#endif
    ImGui::Separator();

    b8 grid_trigger_pressed = ui::menu_item(
        (const char *)VIEWPORT_CONTEXT_MENU_GRID_LABEL.buff,
                                            ICON_FA_CHEVRON_RIGHT,
                                            nullptr,
                                            true,
                                            false);
    b8     grid_trigger_hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
    ImVec2 grid_item_min        = ImGui::GetItemRectMin();
    ImVec2 grid_item_max        = ImGui::GetItemRectMax();

    b8 submenu_popup_open =
        ImGui::IsPopupOpen((const char *)VIEWPORT_CONTEXT_MENU_GRID_POPUP_ID.buff,
                           ImGuiPopupFlags_None);
    b8 submenu_target_open =
        submenu_popup_open || grid_trigger_hovered || grid_trigger_pressed;
    f32 submenu_alpha = ui::anim::track_popup_alpha(
        storage,
        ImGui::GetID((const char *)VIEWPORT_CONTEXT_MENU_GRID_ANIM_ID.buff),
        submenu_target_open,
        delta_time);

    if (grid_trigger_hovered || grid_trigger_pressed || submenu_popup_open)
    {
        ImGui::SetNextWindowPos(ImVec2(grid_item_max.x + 2.0f, grid_item_min.y));
    }
    if (grid_trigger_hovered || grid_trigger_pressed)
    {
        ImGui::OpenPopup((const char *)VIEWPORT_CONTEXT_MENU_GRID_POPUP_ID.buff);
    }

    ImGuiID submenu_close_timer_id =
        ImGui::GetID((const char *)VIEWPORT_CONTEXT_MENU_GRID_CLOSE_TIMER_ID.buff);
    f32 *submenu_close_timer = storage->GetFloatRef(submenu_close_timer_id, 0.0f);
    b8   close_context_menu  = false;

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.9f * submenu_alpha);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, menu_bg_col);
    if (ImGui::BeginPopup((const char *)VIEWPORT_CONTEXT_MENU_GRID_POPUP_ID.buff))
    {
        b8 submenu_hovered =
            ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);

        for (u32 i = 0; i < ARRAY_COUNT(GRID_SIZE_PRESETS); ++i)
        {
            const Grid_Size_Preset &preset = GRID_SIZE_PRESETS[i];
            b8 selected = viewport_component_spacing_matches(
                state->grid_spacing,
                preset.spacing_mm);

            if (ui::menu_item((const char *)preset.label.buff, nullptr, &selected))
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

#ifdef DEBUG_BUILD
INTERNAL_FUNC vec3
viewport_component_debug_camera_forward(const Editor_Layer_State *state)
{
    f32 yaw_radians   = state->debug_camera.yaw_degrees * math::DEG_RAD_FACTOR;
    f32 pitch_radians = state->debug_camera.pitch_degrees * math::DEG_RAD_FACTOR;

    vec3 forward = {math_cos(yaw_radians) * math_cos(pitch_radians),
                    math_sin(pitch_radians),
                    math_sin(yaw_radians) * math_cos(pitch_radians)};
    return vec3_norm_copy(forward);
}

INTERNAL_FUNC f32
viewport_component_sanitize_wheel_delta(f32 raw_delta)
{
    if (math_abs_value(raw_delta) < VIEWPORT_DEBUG_CAMERA_WHEEL_DEADZONE)
    {
        return 0.0f;
    }

    return CLAMP(raw_delta,
                 -VIEWPORT_DEBUG_CAMERA_MAX_WHEEL_DELTA,
                 VIEWPORT_DEBUG_CAMERA_MAX_WHEEL_DELTA);
}

INTERNAL_FUNC b8
viewport_component_debug_camera_position_valid(vec3 p)
{
    // NaN checks (x != x means NaN), then finite bounds.
    if (p.x != p.x || p.y != p.y || p.z != p.z)
    {
        return false;
    }

    return math_abs_value(p.x) <= VIEWPORT_DEBUG_CAMERA_MAX_POSITION_ABS &&
           math_abs_value(p.y) <= VIEWPORT_DEBUG_CAMERA_MAX_POSITION_ABS &&
           math_abs_value(p.z) <= VIEWPORT_DEBUG_CAMERA_MAX_POSITION_ABS;
}

INTERNAL_FUNC vec3
viewport_component_debug_camera_clamped_position(vec3 p)
{
    if (p.x != p.x || p.y != p.y || p.z != p.z)
    {
        return vec3_zero();
    }

    p.x = CLAMP(p.x,
                -VIEWPORT_DEBUG_CAMERA_MAX_POSITION_ABS,
                VIEWPORT_DEBUG_CAMERA_MAX_POSITION_ABS);
    p.y = CLAMP(p.y,
                -VIEWPORT_DEBUG_CAMERA_MAX_POSITION_ABS,
                VIEWPORT_DEBUG_CAMERA_MAX_POSITION_ABS);
    p.z = CLAMP(p.z,
                -VIEWPORT_DEBUG_CAMERA_MAX_POSITION_ABS,
                VIEWPORT_DEBUG_CAMERA_MAX_POSITION_ABS);
    return p;
}

INTERNAL_FUNC vec3
viewport_component_debug_camera_right(const Editor_Layer_State *state)
{
    vec3 forward = viewport_component_debug_camera_forward(state);
    return vec3_norm_copy(vec3_cross(forward, vec3_up()));
}

INTERNAL_FUNC vec3
viewport_component_debug_camera_up(const Editor_Layer_State *state)
{
    vec3 right   = viewport_component_debug_camera_right(state);
    vec3 forward = viewport_component_debug_camera_forward(state);
    return vec3_norm_copy(vec3_cross(right, forward));
}

INTERNAL_FUNC mat4
viewport_component_debug_camera_projection(const Editor_Layer_State *state)
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

    f32 aspect_ratio = vp_w / vp_h;
    return mat4_project_perspective(VIEWPORT_DEBUG_CAMERA_FOV_RADIANS,
                                    aspect_ratio,
                                    VIEWPORT_DEBUG_CAMERA_NEAR,
                                    VIEWPORT_DEBUG_CAMERA_FAR);
}

INTERNAL_FUNC mat4
viewport_component_debug_camera_view(const Editor_Layer_State *state)
{
    vec3 forward = viewport_component_debug_camera_forward(state);
    vec3 up      = viewport_component_debug_camera_up(state);
    return mat4_look_at(state->debug_camera.position,
                        state->debug_camera.position + forward,
                        up);
}

INTERNAL_FUNC b8
viewport_component_debug_pick_orbit_pivot(const Editor_Layer_State *state,
                                          vec3                     *out_pivot)
{
    if (!state || !out_pivot)
    {
        return false;
    }

    ImVec2 mouse = ImGui::GetIO().MousePos;
    if (!viewport_component_is_mouse_inside_viewport_image(state, mouse))
    {
        return false;
    }

    f32 width  = state->viewport_image_size.x;
    f32 height = state->viewport_image_size.y;
    if (width < 1.0f || height < 1.0f)
    {
        return false;
    }

    f32 local_x = mouse.x - state->viewport_image_pos.x;
    f32 local_y = mouse.y - state->viewport_image_pos.y;
    f32 ndc_x   = (local_x / width) * 2.0f - 1.0f;
    f32 ndc_y   = 1.0f - (local_y / height) * 2.0f;

    f32 aspect_ratio = width / height;
    f32 tan_half_fov = math_tan(VIEWPORT_DEBUG_CAMERA_FOV_RADIANS * 0.5f);

    vec3 right   = viewport_component_debug_camera_right(state);
    vec3 up      = viewport_component_debug_camera_up(state);
    vec3 forward = viewport_component_debug_camera_forward(state);
    vec3 ray_dir =
        forward + vec3_scale(right, ndc_x * aspect_ratio * tan_half_fov) +
        vec3_scale(up, ndc_y * tan_half_fov);
    ray_dir = vec3_norm_copy(ray_dir);

    f32 dir_z = ray_dir.z;
    if (math_abs_value(dir_z) < VIEWPORT_DEBUG_CAMERA_PICK_MIN_DIR_Z)
    {
        return false;
    }

    f32 t = (VIEWPORT_DEBUG_CAMERA_ORBIT_PLANE_Z - state->debug_camera.position.z) /
            dir_z;
    if (t <= 0.0f || t > VIEWPORT_DEBUG_CAMERA_MAX_PICK_DISTANCE)
    {
        return false;
    }

    vec3 pivot = state->debug_camera.position + vec3_scale(ray_dir, t);
    if (!viewport_component_debug_camera_position_valid(pivot))
    {
        return false;
    }

    *out_pivot = pivot;
    return true;
}
#endif
