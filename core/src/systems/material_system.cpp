#include "systems/material_system.hpp"
#include "core/logger.hpp"
#include "core/thread_context.hpp"
#include "data_structures/hashmap.hpp"
#include "defines.hpp"
#include "utils/string.hpp"
#include "math/math.hpp"
#include "memory/memory.hpp"
#include "renderer/renderer_frontend.hpp"

#include "systems/resource_system.hpp"
#include "systems/texture_system.hpp"

internal_var Material_System_State *state_ptr;

INTERNAL_FUNC b8   create_default_material(Material_System_State *state);
INTERNAL_FUNC void destroy_material(Material *material);
INTERNAL_FUNC b8   load_material(Material_Config config, Material *material);

Material *material_system_acquire_from_config(Material_Config config);

Material_System_State *
material_system_init(Arena *allocator, Material_System_Config config)
{
    u32 count = config.max_material_count;

    RUNTIME_ASSERT_MSG(
        count > 0,
        "texture_system_initialize - config.max_texture_count must be > 0");

    auto *state = push_struct(allocator, Material_System_State);

    state->config = config;

    state->material_registry.init(allocator, count);

    // Already zeroed out by the allocate function
    state->registered_materials = push_array(allocator, Material, count);

    // Invalidate all ids present in the material array
    for (u32 i = 0; i < count; ++i)
    {
        state->registered_materials[i].id         = INVALID_ID;
        state->registered_materials[i].generation = INVALID_ID;
    }

    create_default_material(state);

    state_ptr = state;

    return state;
}

void
material_system_shutdown()
{

    u32 max_count = state_ptr->config.max_material_count;

    CORE_INFO("Destroying registered materials...");
    // Destroy all internal renderer-specific resources for texture that are
    // still valid in the registry
    for (u64 i = 0; i < max_count; ++i)
    {
        Material *material = &state_ptr->registered_materials[i];

        if (material->id != INVALID_ID)
        {
            destroy_material(material);
            CORE_INFO("Material destroyed.");
        }
    }

    // Release default texture resources
    destroy_material(&state_ptr->default_material);
}

Material *
material_system_acquire(const char *name)
{
    Scratch_Arena scratch = scratch_begin(nullptr, 0);

    Resource resource = {};

    if (!resource_system_load(scratch.arena,
                              name,
                              Resource_Type::MATERIAL,
                              &resource))
    {
        CORE_ERROR(
            "Failed to load material file: '%s'. Null pointer will be "
            "returned.",
            name);
        scratch_end(scratch);
        return nullptr;
    }

    Material *material = nullptr;
    if (resource.data)
    {
        material = material_system_acquire_from_config(
            *static_cast<Material_Config *>(resource.data));
    }

    scratch_end(scratch);

    if (!material)
    {
        CORE_ERROR("Failed to load material resource, returning nullptr");
    }

    return material;
}

Material *
material_system_acquire_from_config(Material_Config config)
{
    if (str_match(config.name,
                  str_from_cstr(DEFAULT_MATERIAL_NAME),
                  String_Match_Flags::CASE_INSENSITIVE))
    {
        return &state_ptr->default_material;
    }

    Material_Reference ref;
    Material          *material = nullptr;

    if (state_ptr->material_registry.find(config.name, &ref))
    {
        CORE_DEBUG(
            "Material '%.*s' already present in the registry. Returning...",
            (int)config.name.size,
            (const char *)config.name.data);
        ref.reference_count++;
        material = &state_ptr->registered_materials[ref.handle];
    }
    else
    {
        CORE_DEBUG("Material '%.*s' not present in the registry. Loading...",
                   (int)config.name.size,
                   (const char *)config.name.data);

        // Find the index for the material to be stored
        u32 index = 0;
        for (u32 i = 0; i < state_ptr->config.max_material_count; ++i)
        {
            // If we find a slot in the memory that has a valid id means that
            // slot is empty and we can use it
            if (state_ptr->registered_materials[i].id == INVALID_ID)
            {
                material = &state_ptr->registered_materials[i];
                index    = i;
                break;
            }
        }

        // Handle the case when we loop over and do not find an empty slot
        if (!material)
        {
            CORE_FATAL(
                "Material registry is full and cannot store any additional "
                "materials");
            return nullptr;
        }

        // Material not present so we need to load first
        if (!load_material(config, material))
        {
            CORE_ERROR("Failed to load material '%.*s'",
                       (int)config.name.size,
                       (const char *)config.name.data);
            return nullptr;
        }

        material->id = index;

        ref.handle          = index;
        ref.auto_release    = config.auto_release;
        ref.reference_count = 1;
    }

    state_ptr->material_registry.add(config.name, &ref, true);

    return material;
}

void
material_system_release(const char *name)
{

    if (str_match(str_from_cstr(name),
                  str_from_cstr(DEFAULT_MATERIAL_NAME),
                  String_Match_Flags::CASE_INSENSITIVE))
    {
        CORE_WARN(
            "material_system_release - Called for default texture. "
            "Skipping...");
        return;
    }

    Material_Reference ref;

    if (state_ptr->material_registry.find(str_from_cstr(name), &ref))
    {
        ref.reference_count--;

        char name_copy[MATERIAL_NAME_MAX_LENGTH];
        u64  len = str_from_cstr(name).size;
        memory_copy(name_copy, name, len);
        name_copy[len] = '\0';

        if (ref.reference_count == 0 && ref.auto_release)
        {

            CORE_INFO(
                "material_system_release - Material '%s' has 0 remaining "
                "references and is marked as 'auto_release'. Releasing from "
                "registry...",
                name_copy);

            destroy_material(&state_ptr->registered_materials[ref.handle]);
            CORE_DEBUG("Resources of material destroyed from renderer");

            if (!state_ptr->material_registry.remove(str_from_cstr(name_copy)))
            {
                CORE_FATAL("Error while removing material from registry");
            }

            return;
        }

        // Update material reference with the new reference count
        state_ptr->material_registry.add(
            str_from_cstr(name_copy), &ref, true);
    }
    else
    {
        CORE_DEBUG("Material '%s' not present in the registry. Skipping...",
                   name);
    }
}

INTERNAL_FUNC b8
create_default_material(Material_System_State *state)
{
    memory_zero(&state->default_material, sizeof(Material));

    state->default_material.id         = INVALID_ID;
    state->default_material.generation = INVALID_ID;
    state->default_material.name =
        const_str_from_cstr<MATERIAL_NAME_MAX_LENGTH>(DEFAULT_MATERIAL_NAME);

    state->default_material.diffuse_color    = vec4_one();
    state->default_material.diffuse_map.type = Texture_Type::MAP_DIFFUSE;
    state->default_material.diffuse_map.texture =
        texture_system_get_default_texture();

    if (!renderer_create_material(&state->default_material))
    {
        CORE_FATAL("Failed to create default material");
        return false;
    }

    return true;
}

INTERNAL_FUNC void
destroy_material(Material *material)
{
    CORE_TRACE("Destroying material '%.*s'",
               (int)material->name.size,
               (const char *)material->name.data);

    if (material->diffuse_map.texture)
    {
        texture_system_release(
            (const char *)material->diffuse_map.texture->name.data);
    }

    renderer_destroy_material(material);
    memory_zero(material, sizeof(Material));
    material->id          = INVALID_ID;
    material->generation  = INVALID_ID;
    material->internal_id = INVALID_ID;
}

INTERNAL_FUNC b8
load_material(Material_Config config, Material *material)
{
    memory_zero(material, sizeof(Material));

    material->name = config.name;

    material->diffuse_color = config.diffuse_color;

    if (config.diffuse_map_name.size > 0)
    {

        material->diffuse_map.type = Texture_Type::MAP_DIFFUSE;
        material->diffuse_map.texture =
            texture_system_acquire(
                (const char *)config.diffuse_map_name.data);

        if (!material->diffuse_map.texture)
        {
            CORE_WARN(
                "Failed to load texture '%.*s' for material '%.*s', using "
                "default.",
                (int)config.diffuse_map_name.size,
                (const char *)config.diffuse_map_name.data,
                (int)config.name.size,
                (const char *)config.name.data);

            material->diffuse_map.texture =
                texture_system_get_default_texture();
        }
    }
    else
    {
        material->diffuse_map.type    = Texture_Type::UNKNOWN;
        material->diffuse_map.texture = nullptr;
    }

    if (!renderer_create_material(material))
    {
        CORE_ERROR("Failed to acquire renderer resources for material '%.*s'",
                   (int)config.name.size,
                   (const char *)config.name.data);
        return false;
    }

    return true;
}

Material *
material_system_get_default()
{
    return &state_ptr->default_material;
}
