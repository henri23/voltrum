#include "systems/material_system.hpp"
#include "core/logger.hpp"
#include "data_structures/hashmap.hpp"
#include "defines.hpp"
#include "math/math.hpp"
#include "memory/memory.hpp"
#include "renderer/renderer_frontend.hpp"

#include "systems/resource_system.hpp"
#include "systems/texture_system.hpp"

struct Material_Reference {
    Material_ID handle;
    u64 reference_count;
    b8 auto_release;
};

struct Material_System_State {
    Material_System_Config config;
    Material default_material;

    Hashmap<Material_Reference> material_registry;
    Material* registered_materials;
};

// TODO: Allocate with allocator
internal_var Material_System_State state = {};

INTERNAL_FUNC b8 create_default_material(Material_System_State* state);
INTERNAL_FUNC void destroy_material(Material* material);
INTERNAL_FUNC b8 load_material(Material_Config config, Material* material);

Material* material_system_acquire_from_config(Material_Config config);

b8 material_system_init(Material_System_Config config) {
    u32 count = config.max_material_count;
    RUNTIME_ASSERT_MSG(count > 0,
        "texture_system_initialize - config.max_texture_count must be > 0");

    state.config = config;
    state.material_registry.init(count);

    // Already zeroed out by the allocate function
    state.registered_materials = static_cast<Material*>(
        memory_allocate(sizeof(Material) * count, Memory_Tag::ARRAY));

    // Invalidate all ids present in the material array
    for (u32 i = 0; i < count; ++i) {
        state.registered_materials[i].id = INVALID_ID;
        state.registered_materials[i].generation = INVALID_ID;
    }

    create_default_material(&state);

    return true;
}

void material_system_shutdown() {

    u32 max_count = state.config.max_material_count;

    CORE_INFO("Destroying registered materials...");
    // Destroy all internal renderer-specific resources for texture that are
    // still valid in the registry
    for (u64 i = 0; i < max_count; ++i) {
        Material* material = &state.registered_materials[i];

        if (material->id != INVALID_ID) {
            char name[MATERIAL_NAME_MAX_LENGTH];
            string_copy(name, material->name);
            destroy_material(material);
            CORE_INFO("Material '%s' destroyed.", name);
        }
    }

    // Deallocate the memory of the registry
    memory_deallocate(state.registered_materials,
        sizeof(Material) * state.config.max_material_count,
        Memory_Tag::ARRAY);

    // Release default texture resources
    destroy_material(&state.default_material);

    state.material_registry.free();

    memory_zero(&state, sizeof(Material_System_State));
}

Material* material_system_acquire(const char* name) {
    Resource resource = {};

    if (!resource_system_load(name, Resource_Type::MATERIAL, &resource)) {
        CORE_ERROR(
            "Failed to load material file: '%s'. Null pointer will be "
            "returned.",
            name);
        return nullptr;
    }

    Material* material = nullptr;
    if (resource.data) {
        material = material_system_acquire_from_config(
            *static_cast<Material_Config*>(resource.data));
    }

    resource_system_unload(&resource);

    if (!material) {
        CORE_ERROR("Failed to load material resource, returning nullptr");
    }

    return material;
}

Material* material_system_acquire_from_config(Material_Config config) {
    if (string_check_equal_insensitive(config.name, DEFAULT_MATERIAL_NAME)) {
        return &state.default_material;
    }

    Material_Reference ref;
    Material* material = nullptr;

    if (state.material_registry.find(config.name, &ref)) {
        CORE_DEBUG(
            "Material '%s' already present in the registry. Returning...",
            config.name);
        ref.reference_count++;
        material = &state.registered_materials[ref.handle];
    } else {
        CORE_DEBUG("Material '%s' not present in the registry. Loading...",
            config.name);

        // Find the index for the material to be stored
        u32 index = 0;
        for (u32 i = 0; i < state.config.max_material_count; ++i) {
            // If we find a slot in the memory that has a valid id means that
            // slot is empty and we can use it
            if (state.registered_materials[i].id == INVALID_ID) {
                material = &state.registered_materials[i];
                index = i;
                break;
            }
        }

        // Handle the case when we loop over and do not find an empty slot
        if (!material) {
            CORE_FATAL(
                "Material registry is full and cannot store any additional "
                "materials");
            return nullptr;
        }

        // Material not present so we need to load first
        if (!load_material(config, material)) {
            CORE_ERROR("Failed to load material '%s'", config.name);
            return nullptr;
        }

        material->id = index;

        ref.handle = index;
        ref.auto_release = config.auto_release;
        ref.reference_count = 1;
    }

    state.material_registry.add(config.name, &ref, true);

    return material;
}

void material_system_release(const char* name) {

    if (string_check_equal_insensitive(name, DEFAULT_MATERIAL_NAME)) {
        CORE_WARN(
            "material_system_release - Called for default texture. "
            "Skipping...");
        return;
    }

    Material_Reference ref;

    if (state.material_registry.find(name, &ref)) {
        ref.reference_count--;

        char name_copy[MATERIAL_NAME_MAX_LENGTH];
        string_copy(name_copy, name);

        if (ref.reference_count == 0 && ref.auto_release) {

            CORE_INFO(
                "material_system_release - Material '%s' has 0 remaining "
                "references and is marked as 'auto_release'. Releasing from "
                "registry...",
                name_copy);

            destroy_material(&state.registered_materials[ref.handle]);
            CORE_DEBUG("Resources of material destroyed from renderer");

            if (!state.material_registry.remove(name_copy)) {
                CORE_FATAL("Error while removing material from registry");
            }

            return;
        }

        // Update material reference with the new reference count
        state.material_registry.add(name_copy, &ref, true);

    } else {
        CORE_DEBUG("Material '%s' not present in the registry. Skipping...",
            name);
    }
}

INTERNAL_FUNC b8 create_default_material(Material_System_State* state) {
    memory_zero(&state->default_material, sizeof(Material));

    state->default_material.id = INVALID_ID;
    state->default_material.generation = INVALID_ID;
    string_ncopy(state->default_material.name,
        DEFAULT_MATERIAL_NAME,
        MATERIAL_NAME_MAX_LENGTH);

    state->default_material.diffuse_color = vec4_one();
    state->default_material.diffuse_map.type = Texture_Type::MAP_DIFFUSE;
    state->default_material.diffuse_map.texture =
        texture_system_get_default_texture();

    if (!renderer_create_material(&state->default_material)) {
        CORE_FATAL("Failed to create default material");
        return false;
    }

    return true;
}

INTERNAL_FUNC void destroy_material(Material* material) {
    CORE_TRACE("Destroying material '%s'", material->name);

    if (material->diffuse_map.texture) {
        texture_system_release(material->diffuse_map.texture->name);
    }

    renderer_destroy_material(material);
    memory_zero(material, sizeof(Material));
    material->id = INVALID_ID;
    material->generation = INVALID_ID;
    material->internal_id = INVALID_ID;
}

INTERNAL_FUNC b8 load_material(Material_Config config, Material* material) {
    memory_zero(material, sizeof(Material));

    string_ncopy(material->name, config.name, MATERIAL_NAME_MAX_LENGTH);

    material->diffuse_color = config.diffuse_color;

    if (string_length(config.diffuse_map_name) > 0) {

        material->diffuse_map.type = Texture_Type::MAP_DIFFUSE;
        material->diffuse_map.texture =
            texture_system_acquire(config.diffuse_map_name);

        if (!material->diffuse_map.texture) {
            CORE_WARN(
                "Failed to load texture '%s' for material '%s', using default.",
                config.diffuse_map_name,
                config.name);

            material->diffuse_map.texture =
                texture_system_get_default_texture();
        }
    } else {
        material->diffuse_map.type = Texture_Type::UNKNOWN;
        material->diffuse_map.texture = nullptr;
    }

    if (!renderer_create_material(material)) {
        CORE_ERROR("Failed to acquire renderer resources for material '%s'",
            config.name);
        return false;
    }

    return true;
}

Material* material_system_get_default() { return &state.default_material; }
