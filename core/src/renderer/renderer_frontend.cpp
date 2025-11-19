#include "renderer/renderer_frontend.hpp"
#include "defines.hpp"
#include "renderer/renderer_backend.hpp"

#include "core/logger.hpp"
#include "math/math.hpp"
#include "math/math_types.hpp"

#include "renderer/renderer_types.hpp"

#include "events/events.hpp"
#include "systems/texture_system.hpp"

// Renderer frontend will manage the backend interface state
struct Renderer_System_State {
    Renderer_Backend backend;
    mat4 projection;

    // Cached value of the camera transformation managed in the client
    mat4 view;

    f32 near_clip = 0.1f;
    f32 far_clip = 1000.0f;

    // WARN: Temp
    Texture* test_diffuse; // The actual texture to load to an object
};

internal_variable Renderer_System_State state;

// WARN: temp block
INTERNAL_FUNC b8 event_on_debug_event(const Event* event) {
    const char* names[3] = {"metal", "space_parallax", "yellow_track"};

    local_persist s8 choice = 2;
    choice++;
    choice %= 3;

    state.test_diffuse = texture_system_acquire(names[choice]);
    return true;
}
// WARN: temp block

b8 renderer_startup(const char* application_name) {

    events_register_callback(Event_Type::DEBUG0, event_on_debug_event);

    if (!renderer_backend_initialize(Renderer_Backend_Type::VULKAN,
            &state.backend)) {

        CORE_INFO("Failed to initialize renderer backend");
        return false;
    }

    state.backend.initialize(&state.backend, application_name);

    state.projection = mat4_project_perspective(deg_to_rad(45.0f),
        1280 / 720.0f,
        state.near_clip,
        state.far_clip);

    CORE_DEBUG("Renderer subsystem initialized");
    return true;
}

void renderer_shutdown() {
    events_unregister_callback(Event_Type::DEBUG0, event_on_debug_event);

    state.backend.shutdown(&state.backend);

    renderer_backend_shutdown(&state.backend);

    CORE_DEBUG("Renderer subsystem shutting down...");
}

// TODO: Add a method only for the viewport resize event because this captures
// the window resize, we need to update the aspect ratio on the viewport resize
void renderer_on_resize(u16 width, u16 height) {

    state.projection = mat4_project_perspective(deg_to_rad(45.0f),
        width / (f32)height,
        state.near_clip,
        state.far_clip);

    state.backend.resized(&state.backend, width, height);
}

b8 renderer_begin_frame(f32 delta_t) {
    return state.backend.begin_frame(&state.backend, delta_t);
}

b8 renderer_end_frame(f32 delta_t) {
    b8 result = state.backend.end_frame(&state.backend, delta_t);

    state.backend.frame_number++;
    return result;
}

b8 renderer_draw_frame(Render_Packet* packet) {
    if (renderer_begin_frame(packet->delta_time)) {

        state.backend.update_global_state(state.projection,
            state.view,
            vec3_zero(),
            vec4_one(),
            0);

        local_persist f32 angle = 0.01f;
        angle += 0.01f;
        quaternion rotation =
            quat_from_axis_angle(vec3_forward(), angle, false);
        mat4 model = quat_to_rotation_matrix(rotation, vec3_zero());

        Geometry_Render_Data data;
        data.object_id = 0; // Change to the actual ID of the object
        data.model = model;

        if (state.test_diffuse == nullptr) {
            state.test_diffuse = texture_system_get_default_texture();
        }

        data.textures[0] = state.test_diffuse;

        state.backend.update_object(data);

        b8 result = renderer_end_frame(packet->delta_time);

        if (!result) {
            CORE_ERROR(
                "renderer_end_frame failed. Application shutting down...");
            return false;
        }
    }

    return true;
}

b8 renderer_create_ui_image(u32 width,
    u32 height,
    const void* pixel_data,
    u32 pixel_data_size,
    UI_Image_Resource* out_image_resource) {
    return state.backend.create_ui_image(&state.backend,
        width,
        height,
        pixel_data,
        pixel_data_size,
        out_image_resource);
}

void renderer_destroy_ui_image(UI_Image_Resource* resource) {
    state.backend.destroy_ui_image(&state.backend, resource);
}

void renderer_set_view(mat4 view) { state.view = view; }

void renderer_create_texture(const char* name,
    s32 width,
    s32 height,
    s32 channel_count,
    const u8* pixels,
    b8 has_transparency,
    struct Texture* out_texture) {

    state.backend.create_texture(name,
        width,
        height,
        channel_count,
        pixels,
        has_transparency,
        out_texture);
}

void renderer_destroy_texture(struct Texture* texture) {
    state.backend.destroy_texture(texture);
}
