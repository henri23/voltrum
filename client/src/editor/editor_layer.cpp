#include "editor_layer.hpp"

#include "input/input.hpp"
#include "input/input_codes.hpp"
#include <core/logger.hpp>
#include <imgui.h>
#include <math/math.hpp>
#include <memory/memory.hpp>
#include <renderer/renderer_frontend.hpp>

INTERNAL_FUNC void viewport_camera_initialize(Viewport_Camera* camera,
    vec3 position = {0, 0, 10.0f});
INTERNAL_FUNC void viewport_camera_recalculate_view(Viewport_Camera* camera);
INTERNAL_FUNC void viewport_camera_rotate_yaw(Viewport_Camera* camera,
    f32 amount);
INTERNAL_FUNC void viewport_camera_rotate_pitch(Viewport_Camera* camera,
    f32 amount);
INTERNAL_FUNC b8 viewport_camera_update(Viewport_Camera* camera,
    f32 delta_time,
    b8 viewport_active);

INTERNAL_FUNC void render_viewport_window(Editor_Layer_State* state,
    f32 delta_time);

void editor_layer_on_attach(UI_Layer* self) {
    self->state =
        memory_allocate(sizeof(Editor_Layer_State), Memory_Tag::CLIENT);

    Editor_Layer_State* state = (Editor_Layer_State*)self->state;

    viewport_camera_initialize(&state->camera, {0, 0, 10.0f});
    viewport_camera_recalculate_view(&state->camera);
    renderer_set_view(state->camera.view_matrix);

    state->viewport_focused = false;
    state->viewport_hovered = false;

    u32 width = 0;
    u32 height = 0;
    renderer_get_viewport_size(&width, &height);
    state->viewport_size = {(f32)width, (f32)height};
    state->last_viewport_size = state->viewport_size;

    CLIENT_INFO("Editor layer attached");
}

void editor_layer_on_detach(UI_Layer* self) {
    if (self->state) {
        memory_deallocate(self->state,
            sizeof(Editor_Layer_State),
            Memory_Tag::CLIENT);
        self->state = nullptr;
    }

    CLIENT_INFO("Editor layer detached");
}

b8 editor_layer_on_update(UI_Layer* self, f32 delta_time) {
    Editor_Layer_State* state = (Editor_Layer_State*)self->state;

    b8 viewport_active = state->viewport_hovered;
    // b8 viewport_active = state->viewport_focused || state->viewport_hovered;

    b8 camera_moved =
        viewport_camera_update(&state->camera, delta_time, viewport_active);

    if (camera_moved) {
        renderer_set_view(state->camera.view_matrix);
    }

    return true;
}

b8 editor_layer_on_render(UI_Layer* self, f32 delta_time) {
    Editor_Layer_State* state = (Editor_Layer_State*)self->state;

    render_viewport_window(state, delta_time);

    return true;
}

INTERNAL_FUNC void viewport_camera_initialize(Viewport_Camera* camera,
    vec3 position) {
    camera->position = position;
    camera->euler_angles = vec3_zero();
    camera->camera_matrix = mat4_translation(camera->position);
    camera->view_matrix = mat4_inv(camera->camera_matrix);
    camera->view_dirty = true;
}

INTERNAL_FUNC void viewport_camera_recalculate_view(Viewport_Camera* camera) {
    if (camera->view_dirty) {
        mat4 rotation = mat4_euler_xyz(camera->euler_angles.x,
            camera->euler_angles.y,
            camera->euler_angles.z);
        mat4 translation = mat4_translation(camera->position);
        camera->camera_matrix = rotation * translation;
        camera->view_matrix = mat4_inv(camera->camera_matrix);
        camera->view_dirty = false;
    }
}

INTERNAL_FUNC void viewport_camera_rotate_yaw(Viewport_Camera* camera,
    f32 amount) {
    camera->euler_angles.y += amount;
    camera->view_dirty = true;
}

INTERNAL_FUNC void viewport_camera_rotate_pitch(Viewport_Camera* camera,
    f32 amount) {
    camera->euler_angles.x += amount;
    f32 limit = deg_to_rad(89.0f);
    camera->euler_angles.x = CLAMP(camera->euler_angles.x, -limit, limit);
    camera->view_dirty = true;
}

INTERNAL_FUNC b8 viewport_camera_update(Viewport_Camera* camera,
    f32 delta_time,
    b8 viewport_active) {
    b8 camera_moved = false;

    if (viewport_active) {
        f32 rotation_velocity = 100.0f;
        f32 rotation_delta = rotation_velocity * delta_time;

        if (input_is_key_pressed(Key_Code::A) ||
            input_is_key_pressed(Key_Code::LEFT)) {
            viewport_camera_rotate_yaw(camera, rotation_delta);
            camera_moved = true;
        }
        if (input_is_key_pressed(Key_Code::D) ||
            input_is_key_pressed(Key_Code::RIGHT)) {
            viewport_camera_rotate_yaw(camera, -rotation_delta);
            camera_moved = true;
        }
        if (input_is_key_pressed(Key_Code::UP)) {
            viewport_camera_rotate_pitch(camera, rotation_delta);
            camera_moved = true;
        }
        if (input_is_key_pressed(Key_Code::DOWN)) {
            viewport_camera_rotate_pitch(camera, -rotation_delta);
            camera_moved = true;
        }

        f32 movement_speed = 0.5f;
        vec3 velocity = vec3_zero();

        if (input_is_key_pressed(Key_Code::W)) {
            vec3 forward = mat4_forward(camera->camera_matrix);
            velocity = velocity + forward;
        }
        if (input_is_key_pressed(Key_Code::S)) {
            vec3 forward = mat4_backward(camera->camera_matrix);
            velocity = velocity + forward;
        }
        if (input_is_key_pressed(Key_Code::Q)) {
            vec3 left = mat4_left(camera->camera_matrix);
            velocity = velocity + left;
        }
        if (input_is_key_pressed(Key_Code::E)) {
            vec3 right = mat4_right(camera->camera_matrix);
            velocity = velocity + right;
        }
        if (input_is_key_pressed(Key_Code::SPACE)) {
            velocity.y += 1.0f;
        }
        if (input_is_key_pressed(Key_Code::X)) {
            velocity.y -= 1.0f;
        }

        vec3 zero = vec3_zero();
        if (!vec3_are_equal(zero, velocity, 0.0002f)) {
            vec3_norm(&velocity);
            camera->position.x += velocity.x * movement_speed;
            camera->position.y += velocity.y * movement_speed;
            camera->position.z += velocity.z * movement_speed;
            camera->view_dirty = true;
            camera_moved = true;
        }
    }

    if (camera->view_dirty) {
        viewport_camera_recalculate_view(camera);
        camera_moved = true;
    }

    return camera_moved;
}

INTERNAL_FUNC void render_viewport_window(Editor_Layer_State* state,
    f32 delta_time) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewport");
    ImGui::PopStyleVar();

    state->viewport_focused = ImGui::IsWindowFocused();
    state->viewport_hovered = ImGui::IsWindowHovered();

    ImVec2 content_size = ImGui::GetContentRegionAvail();
    state->viewport_size = {content_size.x, content_size.y};

    if (state->viewport_size.x != state->last_viewport_size.x ||
        state->viewport_size.y != state->last_viewport_size.y) {

        u32 width =
            (u32)(state->viewport_size.x < 1.0f ? 1.0f
                                                : state->viewport_size.x);
        u32 height =
            (u32)(state->viewport_size.y < 1.0f ? 1.0f
                                                : state->viewport_size.y);

        CLIENT_DEBUG("Viewport window resized to %ux%u", width, height);

        renderer_resize_viewport(width, height);

        state->last_viewport_size = state->viewport_size;
    }

    // Render viewport and get the current frame's descriptor
    renderer_render_viewport();

    ImGui::Image(renderer_get_rendered_viewport(),
        content_size,
        ImVec2(0, 0),
        ImVec2(1, 1));

    ImGui::End();
}

UI_Layer create_editor_layer() {
    UI_Layer layer = {};
    layer.state = nullptr;
    layer.on_attach = editor_layer_on_attach;
    layer.on_detach = editor_layer_on_detach;
    layer.on_update = editor_layer_on_update;
    layer.on_render = editor_layer_on_render;
    return layer;
}
