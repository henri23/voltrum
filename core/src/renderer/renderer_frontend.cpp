#include "renderer/renderer_frontend.hpp"
#include "defines.hpp"
#include "renderer/renderer_backend.hpp"

#include "core/logger.hpp"
#include "math/math.hpp"
#include "math/math_types.hpp"

#include "renderer/renderer_types.hpp"

// Renderer frontend will manage the backend interface state
struct Renderer_System_State {
    Renderer_Backend backend;
    mat4 projection;

    // Cached value of the camera transformation managed in the client
    mat4 view;

    f32 near_clip = 0.1f;
    f32 far_clip = 1000.0f;
};

internal_var Renderer_System_State state;

b8 renderer_startup(const char* application_name) {

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

        u32 count = packet->geometry_count;
        for (u32 i = 0; i < count; ++i) {
            state.backend.draw_geometry(packet->geometries[i]);
        }

        b8 result = renderer_end_frame(packet->delta_time);

        if (!result) {
            CORE_ERROR(
                "renderer_end_frame failed. Application shutting down...");
            return false;
        }
    }

    return true;
}

void renderer_set_view(mat4 view) { state.view = view; }

void renderer_create_texture(const u8* pixels, struct Texture* texture) {

    state.backend.create_texture(pixels, texture);
}

void renderer_destroy_texture(struct Texture* texture) {
    state.backend.destroy_texture(texture);
}

b8 renderer_create_material(struct Material* material) {
    return state.backend.create_material(material);
}

void renderer_destroy_material(struct Material* material) {
    return state.backend.destroy_material(material);
}

b8 renderer_create_geometry(Geometry* geometry,
    u32 vertex_count,
    const vertex_3d* vertices,
    u32 index_count,
    u32* indices) {

    return state.backend.create_geometry(geometry,
        vertex_count,
        vertices,
        index_count,
        indices);
}

void renderer_destroy_geometry(Geometry* geometry) {
    state.backend.destroy_geometry(geometry);
}
