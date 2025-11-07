#include "renderer/renderer_frontend.hpp"
#include "defines.hpp"
#include "renderer/renderer_backend.hpp"

#include "core/logger.hpp"
#include "math/math.hpp"
#include "math/math_types.hpp"

// Renderer frontend will manage the backend interface state
struct Renderer_System_State {
    Renderer_Backend backend;
};

internal_variable Renderer_System_State state;

b8 renderer_startup(const char* application_name) {

    if (!renderer_backend_initialize(Renderer_Backend_Type::VULKAN,
            &state.backend)) {

        CORE_INFO("Failed to initialize renderer backend");
        return false;
    }

    state.backend.initialize(&state.backend, application_name);

    CORE_DEBUG("Renderer subsystem initialized");
    return true;
}

void renderer_shutdown() {
    state.backend.shutdown(&state.backend);

    renderer_backend_shutdown(&state.backend);

    CORE_DEBUG("Renderer subsystem shutting down...");
}

void renderer_on_resize(u16 width, u16 height) {

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

        // TODO: test temp
        mat4 projection =
            mat4_project_perspective(deg_to_rad(45.0f), 1.0f, 0.1f, 1000.0f);
        local_persist f32 z = -1.0f;
        z -= 0.01f;
        vec3 vec = {0, 0, -30.0f};
        mat4 view = mat4_translation(vec);

        state.backend.update_global_state(projection,
            view,
            vec3_zero(),
            vec4_one(),
            0);

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
