#include "systems/texture_system.hpp"

#include "core/logger.hpp"
#include "core/string.hpp"
#include "data_structures/hashmap.hpp"
#include "memory/memory.hpp"

#include "renderer/renderer_frontend.hpp"

#define STB_IMAGE_IMPLEMENTATION // Already defined in ui_titlebar.cpp
#include "stb_image.h"

struct Texture_System_State {
    Texture_System_Config config;
    Texture default_texture;

    Hashmap<Texture> registered_textures;
    u32 next_texture_id; // Counter for unique texture IDs
};

struct Texture_Reference {
    u64 reference_count;
    u32 handle;
    b8 auto_release;
};

internal_variable Texture_System_State state;

INTERNAL_FUNC b8 create_default_textures(Texture_System_State* state);
INTERNAL_FUNC void destroy_default_textures(Texture_System_State* state);

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
            temp_texture.width,
            temp_texture.height,
            temp_texture.channel_count,
            data,
            has_transparency,
            &temp_texture);

        // Copy old texture
        Texture old_texture = *texture;
        *texture = temp_texture;

        if (old_texture.internal_data)
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

b8 texture_system_init(Texture_System_Config config) {

    if (config.max_texture_count == 0) {
        CORE_FATAL(
            "texture_system_initialize - config.max_texture_count must be > 0");
    }

    state.config = config;
    state.registered_textures.init(config.max_texture_count);
    state.next_texture_id = 0; // Initialize ID counter

    create_default_textures(&state);

    return true;
}

void texture_system_shutdown() {
    // Release texture resources
    Hashmap<Texture>* textures = &state.registered_textures;
    for (u64 i = textures->next_occupied_index(0); i < textures->capacity;
        i = textures->next_occupied_index(i + 1)) {
        Texture* t = &state.registered_textures.memory[i].value;
        if (t->generation != INVALID_ID) {
            renderer_destroy_texture(t);
        }
    }

    destroy_default_textures(&state);

    memory_zero(&state, sizeof(Texture_System_State));
}

// Returns already loaded texture or loads it from disk the first time
Texture* texture_system_acquire(const char* name, b8 auto_release) {

    if (string_check_equal_insensitive(name, DEFAULT_TEXTURE_NAME)) {
        CORE_WARN(
            "texture_system_acquire called for default texture. Use "
            "get_default_texture for texture 'default'");
        return &state.default_texture;
    }

    // NOTE: With this current implementation I do not implmement the
    // auto_release feature properly
    Texture* texture;
    if (state.registered_textures.find(name, &texture)) {
        // Safety measure just in case the texture was invalidated somewhere
        // else
        if (texture->id == INVALID_ID) {
            CORE_WARN(
                "texture_system_acquire - Texture '%s' is registered but its "
                "ID is marked as invalid!",
                name);
            return nullptr;
        }

        CORE_TRACE("Texture '%s' already exists. Returning reference", name);
        return texture;
    }

    // If the registry doesn't contain the texture we need to load it from
    // disk
    // Make sure that there is space for an additional texture
    if (state.registered_textures.full()) {
        CORE_ERROR(
            "Texture registry is full and cannot contain any more "
            "textures");
        return nullptr;
    }

    // If the texture is not found in the registered textures, it means that
    // it needs to be loaded first
    Texture new_texture;
    create_texture(&new_texture); // Initialize to safe defaults
    if (!load_texture(name, &new_texture)) {
        CORE_ERROR("Texture loading failed!");
        return nullptr;
    }

    if (!state.registered_textures.add(name, &new_texture)) {
        CORE_FATAL("Error adding '%s' texture to registry", name);
        return nullptr;
    }

    CORE_TRACE("Texture '%s' did not exist. Successfully loaded from disk",
        name);

    // Return pointer to the newly added texture in the hashmap
    if (!state.registered_textures.find(name, &texture)) {
        CORE_FATAL("Error when retrieving texture '%s' after adding to registry", name);
        return nullptr;
    }

    // Assign unique ID to the texture
    texture->id = state.next_texture_id++;

    return texture;
}

// For the moment I do not need to release the textures since they will be
// destroyed on shutdown
void texture_system_release(const char* name) {}

INTERNAL_FUNC b8 create_default_textures(Texture_System_State* state) {

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
        tex_dimension,
        tex_dimension,
        4,
        pixels,
        false,
        &state->default_texture);

    // Manually set the texture generation to invalid since this is the default
    // texture
    state->default_texture.generation = INVALID_ID;

    // Assign unique ID to default texture
    state->default_texture.id = state->next_texture_id++;

    return true;
}

Texture* texture_system_get_default_texture() { return &state.default_texture; }

INTERNAL_FUNC void destroy_default_textures(Texture_System_State* state) {
    renderer_destroy_texture(&state->default_texture);
}
