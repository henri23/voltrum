#include "systems/texture_system.hpp"

#include "core/asserts.hpp"
#include "core/logger.hpp"
#include "core/thread_context.hpp"
#include "data_structures/hashmap.hpp"
#include "memory/memory.hpp"
#include "utils/string.hpp"

#include "renderer/renderer_frontend.hpp"
#include "systems/resource_system.hpp"

internal_var Texture_System_State *state_ptr;

INTERNAL_FUNC b8   create_default_textures(Texture_System_State *state);
INTERNAL_FUNC void destroy_default_textures(Texture_System_State *state);

INTERNAL_FUNC void
create_texture(Texture *texture)
{
    memory_zero(texture, sizeof(Texture));
    texture->generation = INVALID_ID;
}

INTERNAL_FUNC void destroy_texture(Texture *texture);

INTERNAL_FUNC b8
load_texture(const char *texture_name, Texture *texture, b8 is_ui_texture)
{
    Scratch_Arena scratch = scratch_begin(nullptr, 0);

    Resource img_resource;
    if (!resource_system_load(scratch.arena,
                              texture_name,
                              Resource_Type::IMAGE,
                              &img_resource))
    {
        CORE_ERROR("Failed to load image resource for texture '%s'",
                   texture_name);
        scratch_end(scratch);
        return false;
    }

    Image_Resource_Data *resource_data =
        static_cast<Image_Resource_Data *>(img_resource.data);

    Texture temp_texture;
    temp_texture.width         = resource_data->width;
    temp_texture.height        = resource_data->height;
    temp_texture.channel_count = resource_data->channel_count;

    u32 current_generation = texture->generation;
    texture->generation    = INVALID_ID;

    u64 total_texture_size =
        temp_texture.width * temp_texture.height * temp_texture.channel_count;

    b8 has_transparency = false;

    for (u64 i = 0; i < total_texture_size; i += temp_texture.channel_count)
    {
        u8 alpha = resource_data->pixels[i + 3];
        if (alpha < 255)
        {
            has_transparency = true;
            break;
        }
    }

    temp_texture.name =
        const_str_from_cstr<TEXTURE_NAME_MAX_LENGTH>(texture_name);
    temp_texture.generation       = INVALID_ID;
    temp_texture.has_transparency = has_transparency;

    renderer_create_texture(resource_data->pixels,
                            &temp_texture,
                            is_ui_texture);

    // Copy old texture
    Texture old_texture = *texture;
    *texture            = temp_texture;

    renderer_destroy_texture(&old_texture);

    if (current_generation == INVALID_ID)
    {
        texture->generation = 0;
    }
    else
    {
        texture->generation = current_generation + 1;
    }

    scratch_end(scratch);

    return true;
}

Texture_System_State *
texture_system_init(Arena *allocator, Texture_System_Config config)
{

    u32 count = config.max_texture_count;

    RUNTIME_ASSERT_MSG(
        count > 0,
        "texture_system_initialize - config.max_texture_count must be > 0");

    auto state = push_struct(allocator, Texture_System_State);

    state->config = config;
    state->texture_registry.init(allocator, count);

    state->registered_textures = push_array(allocator, Texture, count);

    // Invalidate all ids present in the texture array
    for (u32 i = 0; i < count; ++i)
    {
        state->registered_textures[i].id         = INVALID_ID;
        state->registered_textures[i].generation = INVALID_ID;
    }

    create_default_textures(state);

    state_ptr = state;

    return state;
}

void
texture_system_shutdown()
{
    ENSURE(state_ptr);

    u32 max_count = state_ptr->config.max_texture_count;

    CORE_INFO("Destroying registered textures...");
    // Destroy all internal renderer-specific resources for texture that are
    // still valid in the registry
    for (u64 i = 0; i < max_count; ++i)
    {
        Texture *texture = &state_ptr->registered_textures[i];

        if (texture->id != INVALID_ID)
        {
            renderer_destroy_texture(texture);
            CORE_INFO("Texture '%.*s' destroyed.",
                      (int)texture->name.size,
                      (const char *)texture->name.data);
        }
    }

    // Release default texture resources
    destroy_default_textures(state_ptr);
}

Texture *
texture_system_acquire(const char *name, b8 auto_release, b8 is_ui_texture)
{
    if (str_match(str_from_cstr(name),
                  str_from_cstr(DEFAULT_TEXTURE_NAME),
                  String_Match_Flags::CASE_INSENSITIVE))
    {
        CORE_WARN(
            "texture_system_acquire - Called for default texture. Use "
            "get_default_texture_method for this");
        return &state_ptr->default_texture;
    }

    Texture_Reference ref;
    Texture          *texture = nullptr;

    if (state_ptr->texture_registry.find(str_from_cstr(name), &ref))
    {
        CORE_DEBUG("Texture '%s' already present in the registry. Returning...",
                   name);
        ref.reference_count++;
        texture = &state_ptr->registered_textures[ref.handle];
    }
    else
    {
        CORE_DEBUG("Texture '%s' not present in the registry. Loading...",
                   name);

        // Find the index for the texture to be stored
        u32 index = 0;
        for (u32 i = 0; i < state_ptr->config.max_texture_count; ++i)
        {
            // If we find a slot in the memory that has a valid id means that
            // slot is empty and we can use it
            if (state_ptr->registered_textures[i].id == INVALID_ID)
            {
                texture = &state_ptr->registered_textures[i];
                index   = i;
                break;
            }
        }

        // Handle the case when we loop over and do not find an empty slot
        if (!texture)
        {
            CORE_FATAL(
                "Texture registry is full and cannot store any additional "
                "textures");
            return nullptr;
        }

        if (!load_texture(name, texture, is_ui_texture))
        {
            CORE_ERROR("Failed to load texture '%s'", name);
            return nullptr;
        }

        // NOTE: texture->generation = 0 will be done inside the load_texture
        // Set the texture ID after the load texture! Because otherwise the
        // loader will give us an INVALID_ID for texture id
        texture->id = index;

        ref.handle          = index;
        ref.auto_release    = auto_release;
        ref.reference_count = 1;
    }

    state_ptr->texture_registry.add(str_from_cstr(name), &ref, true);

    return texture;
}

void
texture_system_release(const char *name)
{

    if (str_match(str_from_cstr(name),
                  str_from_cstr(DEFAULT_TEXTURE_NAME),
                  String_Match_Flags::CASE_INSENSITIVE))
    {
        CORE_WARN(
            "texture_system_release - Called for default texture. Skipping...");
        return;
    }

    Texture_Reference ref;

    if (state_ptr->texture_registry.find(str_from_cstr(name), &ref))
    {
        ref.reference_count--;

        char name_copy[TEXTURE_NAME_MAX_LENGTH];
        // Since we are going to destroy the texture (including its name)
        // but we still need the name to clean the registry, we make a copy
        // of the name, so that the external modules and directly use the
        // texture name to release it, so texture->name as input will still
        // work
        u64 len = str_from_cstr(name).size;
        memory_copy(name_copy, name, len);
        name_copy[len] = '\0';

        if (ref.reference_count == 0 && ref.auto_release)
        {

            CORE_INFO(
                "texture_system_release - Texture '%s' has 0 remaining "
                "references and is marked as 'auto_release'. Releasing from "
                "registry...",
                name_copy);

            destroy_texture(&state_ptr->registered_textures[ref.handle]);
            CORE_DEBUG("Resources of texture destroyed from renderer");

            if (!state_ptr->texture_registry.remove(str_from_cstr(name_copy)))
            {
                CORE_FATAL("Error while removing texture from registry");
            }

            return;
        }

        // Update texture reference with the new reference count
        state_ptr->texture_registry.add(str_from_cstr(name_copy), &ref, true);
    }
    else
    {
        CORE_DEBUG("Texture '%s' not present in the registry. Skipping...",
                   name);
    }
}

INTERNAL_FUNC b8
create_default_textures(Texture_System_State *state)
{

    // NOTE: Create default texture to prevent runtime errors when texture was
    // not found from disk
    CORE_TRACE("Creating default texture...");
    constexpr u32 tex_dimension = 256;
    constexpr u32 bpp           = 4;
    constexpr u32 pixel_count   = tex_dimension * tex_dimension;
    u8            pixels[pixel_count * bpp];
    // Initialize all channels (including alpha) to 255 so the texture is opaque
    memory_set(pixels, 255, sizeof(u8) * pixel_count * bpp);

    for (u64 row = 0; row < tex_dimension; ++row)
    {
        for (u64 col = 0; col < tex_dimension; ++col)
        {
            u64 index     = (row * tex_dimension) + col;
            u64 index_bpp = index * bpp;
            if (row % 2)
            {
                if (col % 2)
                {
                    pixels[index_bpp + 0] = 0;
                    pixels[index_bpp + 1] = 0;
                }
            }
            else
            {
                if (!(col % 2))
                {
                    pixels[index_bpp + 0] = 0;
                    pixels[index_bpp + 1] = 0;
                }
            }
        }
    }

    state->default_texture.name =
        const_str_from_cstr<TEXTURE_NAME_MAX_LENGTH>(DEFAULT_TEXTURE_NAME);

    state->default_texture.width            = tex_dimension;
    state->default_texture.height           = tex_dimension;
    state->default_texture.channel_count    = 4;
    state->default_texture.generation       = INVALID_ID;
    state->default_texture.has_transparency = false;

    renderer_create_texture(pixels, &state->default_texture);

    // Manually set the texture generation to invalid since this is the default
    // texture
    state->default_texture.generation = INVALID_ID;

    return true;
}

Texture *
texture_system_get_default_texture()
{
    return &state_ptr->default_texture;
}

INTERNAL_FUNC void
destroy_default_textures(Texture_System_State *state)
{
    destroy_texture(&state->default_texture);
}

INTERNAL_FUNC void
destroy_texture(Texture *texture)
{
    renderer_destroy_texture(texture);

    memory_zero(texture, sizeof(Texture));
    texture->id         = INVALID_ID;
    texture->generation = INVALID_ID;
}
