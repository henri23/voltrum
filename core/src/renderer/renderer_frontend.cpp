#include "renderer/renderer_frontend.hpp"
#include "defines.hpp"
#include "input/input_codes.hpp"
#include "renderer/renderer_backend.hpp"

#include "core/logger.hpp"
#include "math/math.hpp"
#include "math/math_types.hpp"

#include "renderer/renderer_types.hpp"

// WARN: Temporary code
#include "core/string.hpp"
#include "events/events.hpp"

// #define STB_IMAGE_IMPLEMENTATION // Already defined in ui_titlebar.cpp
#include "stb_image.h"
// WARN: End temp code

// Renderer frontend will manage the backend interface state
struct Renderer_System_State {
    Renderer_Backend backend;
    mat4 projection;

    // Cached value of the camera transformation managed in the client
    mat4 view;

    f32 near_clip = 0.1f;
    f32 far_clip = 1000.0f;

    Texture default_texture;

    // WARN: Temp
    Texture test_diffuse; // The actual texture to load to an object
};

internal_variable Renderer_System_State state;

INTERNAL_FUNC void create_texture(Texture* texture) {
    memory_zero(texture, sizeof(Texture));
    texture->generation = INVALID_ID;
}

INTERNAL_FUNC b8 load_texture(const char* texture_name, Texture* texture) {
    const char* format_str = "assets/textures/%s.%s";
    const s32 required_channel_count = 4; // RGBA

    // NOTE: Most images are stored in a format where the data is actually
    // stored updside-down, so if we load the data from the bottom -> up we
    // should technically retrieve the original orientation
    stbi_set_flip_vertically_on_load(true);
    char full_file_path[512];

    // TODO: Loop over different file extensions
    string_format(full_file_path, format_str, texture_name, "png");

    Texture temp_texture;

    u8* data = stbi_load(full_file_path,
        (s32*)&temp_texture.width,
        (s32*)&temp_texture.height,
        (s32*)&temp_texture.channel_count,
        required_channel_count);

    temp_texture.channel_count = required_channel_count;

    if (data) {
        // Copy over the texture generation, because we might want to use
        // alreaddy loaded texture as target texture in the arguments, so the
        // code should be able to swap out the generation of the already loaded
        // texture
        u32 current_generation = texture->generation;
        texture->generation = INVALID_ID; // When trying to use this texture
                                          // while loading, it will not be valid

        u64 total_texture_size = temp_texture.width * temp_texture.height *
                                 temp_texture.channel_count;

        b8 has_transparency = false;

        // Check for transparency
        for (u64 i = 0; i < total_texture_size; i += required_channel_count) {
            u8 alpha = data[i + 3];
            if (alpha < 255) {
                has_transparency = true;
                break;
            }
        }

        if (stbi_failure_reason()) {
            CORE_WARN(
                "load_texture() in renderer_frontend.cpp failed to load file "
                "'%s' : %s",
                full_file_path,
                stbi_failure_reason());
        }

        renderer_create_texture(texture_name,
            true,
            temp_texture.width,
            temp_texture.height,
            temp_texture.channel_count,
            data,
            has_transparency,
            &temp_texture);

        // Copy old texture
        Texture old_texture = *texture;
        *texture = temp_texture;

        renderer_destroy_texture(&old_texture);

        if (current_generation == INVALID_ID) {
            texture->generation = 0;
        } else {
            texture->generation = current_generation + 1;
        }

        stbi_image_free(data);
        return true;
    } else {
        if (stbi_failure_reason()) {
            CORE_WARN(
                "load_texture() in renderer_frontend.cpp failed to load file "
                "'%s' : %s",
                full_file_path,
                stbi_failure_reason());
        }
        return false;
    }
}

// WARN: temp block
INTERNAL_FUNC b8 event_on_debug_event(const Event* event) {
    const char* names[3] = {"metal", "space_parallax", "yellow_track"};

    local_persist s8 choice = 2;
    choice++;
    choice %= 3;

    load_texture(names[choice], &state.test_diffuse);
    return true;
}
// WARN: temp block

b8 renderer_startup(const char* application_name) {

    state.backend.default_diffuse = &state.default_texture;

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

    // NOTE: Create default texture to prevent runtime errors when texture was
    // not found from disk
    CORE_TRACE("Creating default texture...");
    constexpr u32 tex_dimension = 256;
    constexpr u32 bpp = 4;
    constexpr u32 pixel_count = tex_dimension * tex_dimension;
    u8 pixels[pixel_count * bpp];
    // Initialize all channels (including alpha) to 255 so the texture is opaque
    memory_set(pixels, 255, sizeof(u8) * pixel_count * bpp);

    for (u64 row = 0; row < tex_dimension; ++row) {
        for (u64 col = 0; col < tex_dimension; ++col) {
            u64 index = (row * tex_dimension) + col;
            u64 index_bpp = index * bpp;
            if (row % 2) {
                if (col % 2) {
                    pixels[index_bpp + 0] = 0;
                    pixels[index_bpp + 1] = 0;
                }
            } else {
                if (!(col % 2)) {
                    pixels[index_bpp + 0] = 0;
                    pixels[index_bpp + 1] = 0;
                }
            }
        }
    }

    renderer_create_texture("default",
        false,
        tex_dimension,
        tex_dimension,
        4,
        pixels,
        false,
        &state.default_texture);

    // Manually set the texture generation to invalid since this is the defeault
    // texture
    state.default_texture.generation = INVALID_ID;

    create_texture(&state.test_diffuse);

    CORE_DEBUG("Renderer subsystem initialized");
    return true;
}

void renderer_shutdown() {
    events_unregister_callback(Event_Type::DEBUG0, event_on_debug_event);

    renderer_destroy_texture(&state.default_texture);
    renderer_destroy_texture(&state.test_diffuse);

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
        data.textures[0] = &state.test_diffuse;
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
    b8 auto_release,
    s32 width,
    s32 height,
    s32 channel_count,
    const u8* pixels,
    b8 has_transparency,
    struct Texture* out_texture) {

    state.backend.create_texture(name,
        auto_release,
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
