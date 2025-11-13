#include "app_viewport_layer.hpp"
#include "defines.hpp"
#include "ui/client_ui.hpp"

// Interfaces from core library
#include <core/logger.hpp>
#include <entry.hpp>
#include <events/events.hpp>
#include <input/input.hpp>
#include <input/input_codes.hpp>
#include <math/math.hpp>
#include <memory/memory.hpp>
#include <ui/ui.hpp>
#include <ui/ui_types.hpp>

// WARN: This is temporary, the renderer subsystem should not be
// exposed outside the engine
#include <renderer/renderer_frontend.hpp>

#if defined(PLATFORM_WINDOWS) && !defined(VOLTRUM_STATIC_LINKING)
#    include <imgui.h>
#endif

// Client-specific state structure
struct Frontend_State {
    // Add any client-specific state here later
    b8 initialized;
    // The viewport specific values should be inside the state of the viewport
    // layer but placing them here for simplicity
    mat4 view;
    vec3 camera_position;
    vec3 camera_euler;
    b8 camera_view_dirty;
};

INTERNAL_FUNC void recalculate_view_matrix(Frontend_State* state) {
    if (state->camera_view_dirty) {
        mat4 rotation = mat4_euler_xyz(state->camera_euler.x,
            state->camera_euler.y,
            state->camera_euler.z);

        mat4 translation = mat4_translation(state->camera_position);

        state->view = translation * rotation;
        state->view = mat4_inv(state->view);

        state->camera_view_dirty = false;
    }
}

INTERNAL_FUNC void camera_yaw(Frontend_State* state, f32 yaw_amount) {
    state->camera_euler.y += yaw_amount;
    state->camera_view_dirty = true;
}

INTERNAL_FUNC void camera_pitch(Frontend_State* state, f32 pitch_amount) {
    state->camera_euler.x += pitch_amount;

    // When working with eurler angles for rotations we can suffer from
    // gimbal lock
    f32 limit = deg_to_rad(89.0f);

    state->camera_euler.x = CLAMP(state->camera_euler.x, -limit, limit);
    state->camera_view_dirty = true;
}

// Memory debug callback function
b8 client_memory_debug_callback(const Event* event) {
    if (event->key.key_code == Key_Code::M && !event->key.repeat) {
        u64 allocation_count = memory_get_allocations_count();
        CLIENT_INFO("Current memory allocations: %llu", allocation_count);
    }
    return false; // Don't consume, let other callbacks process
}

// Client lifecycle callback implementations
b8 client_initialize(Client* client_state) {

#if defined(PLATFORM_WINDOWS) && !defined(VOLTRUM_STATIC_LINKING)
    // Get ImGui context from core DLL for Windows compatibility
    // Only needed when using dynamic linking (DLL)
    void* imgui_context = ui_get_imgui_context();
    if (imgui_context) {
        ImGui::SetCurrentContext((ImGuiContext*)imgui_context);
        CLIENT_DEBUG("Set ImGui context from core DLL");
    } else {
        CLIENT_WARN("Failed to get ImGui context from core DLL");
    }
#endif

    events_register_callback(Event_Type::KEY_PRESSED,
        client_memory_debug_callback,
        Event_Priority::LOW);

    Frontend_State* state = (Frontend_State*)client_state->state;

    state->camera_position = {0, 0, 40.0f};
    state->camera_euler = vec3_zero();

    state->view = mat4_translation(state->camera_position);
    state->view = mat4_inv(state->view);

    state->camera_view_dirty = true;

    // Initialize viewport layer
    if (!app_viewport_layer_initialize()) {
        CLIENT_ERROR("Failed to initialize viewport layer");
        return false;
    }

    CLIENT_INFO("Client initialized.");

    return true;
}

b8 client_update(Client* client_state, f32 delta_time) {
    Frontend_State* state = (Frontend_State*)client_state->state;
    static u64 alloc_count = 0;
    u64 prev_alloc_count = alloc_count;

    alloc_count = memory_get_allocations_count();
    if (input_is_key_pressed(Key_Code::M) &&
        input_was_key_pressed(Key_Code::M)) {
        CORE_DEBUG("Allocations: %llu (%llu this frame)",
            alloc_count,
            alloc_count - prev_alloc_count);
    }

    if (input_is_key_pressed(Key_Code::A) ||
        input_is_key_pressed(Key_Code::LEFT)) {
        camera_yaw(state, 1000.0f * delta_time);
    }

    if (input_is_key_pressed(Key_Code::D) ||
        input_is_key_pressed(Key_Code::RIGHT)) {
        camera_yaw(state, -1000.0f * delta_time);
    }

    if (input_is_key_pressed(Key_Code::UP)) {
        camera_pitch(state, 1000.0f * delta_time);
    }

    if (input_is_key_pressed(Key_Code::DOWN)) {
        camera_pitch(state, -1000.0f * delta_time);
    }

    f32 movement_speed = 1.0f;
    vec3 velocity = vec3_zero();

    if (input_is_key_pressed(Key_Code::W)) {
        vec3 forward = mat4_forward(state->view);
        velocity = velocity + forward;
    }

    if (input_is_key_pressed(Key_Code::S)) {
        vec3 forward = mat4_backward(state->view);
        velocity = velocity + forward;
    }

    // Straifing
    if (input_is_key_pressed(Key_Code::Q)) {
        vec3 forward = mat4_left(state->view);
        velocity = velocity + forward;
    }

    if (input_is_key_pressed(Key_Code::E)) {
        vec3 forward = mat4_right(state->view);
        velocity = velocity + forward;
    }

    if (input_is_key_pressed(Key_Code::SPACE)) {
        velocity.y += 1.0f;
    }

    if (input_is_key_pressed(Key_Code::X)) {
        velocity.y -= 1.0f;
    }

    vec3 z = vec3_zero();
    if (!vec3_are_equal(z, velocity, 0.0002f)) {
        vec3_norm(&velocity);
        state->camera_position.x += velocity.x * movement_speed;
        state->camera_position.y += velocity.y * movement_speed;
        state->camera_position.z += velocity.z * movement_speed;

        state->camera_view_dirty = true;
    }

    recalculate_view_matrix(state);

    renderer_set_view(state->view);
    // Client update logic can go here
    return true;
}

b8 client_render(Client* client_state, f32 delta_time) {
    // Client render logic can go here
    return true;
}

void client_on_resize(Client* client_state, u32 width, u32 height) {
    // Handle resize events
}

void client_shutdown(Client* client_state) {
    // Shutdown viewport layer
    app_viewport_layer_shutdown();

    // Clean up frontend state
    memory_deallocate(client_state->state,
        sizeof(Frontend_State),
        Memory_Tag::CLIENT);

    CLIENT_INFO("Client shutdown complete.")
}

// Main client initialization function called by core
b8 create_client(Client* client_state) {
    // Set up client configuration
    client_state->config.name = "Voltrum EDA";
    client_state->config.width = 1600;
    client_state->config.height = 900;
    client_state->config.theme = UI_Theme::CATPPUCCIN_MOCHA;

    // Set up lifecycle callbacks
    client_state->initialize = client_initialize;
    client_state->update = client_update;
    client_state->render = client_render;
    client_state->on_resize = client_on_resize;
    client_state->shutdown = client_shutdown;

    // Initialize state pointers
    client_state->state =
        memory_allocate(sizeof(Frontend_State), Memory_Tag::CLIENT);

    // Create layers using C++17 compatible initialization
    UI_Layer voltrum_layer;
    voltrum_layer.name = "voltrum_window";
    voltrum_layer.on_render = client_ui_render_voltrum_window;
    voltrum_layer.on_attach = nullptr;
    voltrum_layer.on_detach = nullptr;
    voltrum_layer.component_state = nullptr;
    client_state->layers.push_back(voltrum_layer);

    UI_Layer viewport_layer;
    viewport_layer.name = "viewport_layer";
    viewport_layer.on_render = app_viewport_layer_render;
    viewport_layer.on_attach = nullptr;
    viewport_layer.on_detach = nullptr;
    viewport_layer.component_state = nullptr;
    client_state->layers.push_back(viewport_layer);

    client_state->menu_callback = client_ui_render_menus;

    return true;
}
