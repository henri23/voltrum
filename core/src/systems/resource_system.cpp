#include "resource_system.hpp"
#include "core/logger.hpp"
#include "memory/memory.hpp"

// List of system resource loaders that are known to the core library
#include "resources/loaders/binary_loader.hpp"
#include "resources/loaders/font_loader.hpp"
#include "resources/loaders/icon_loader.hpp"
#include "resources/loaders/image_loader.hpp"
#include "resources/loaders/material_loader.hpp"
#include "resources/loaders/text_loader.hpp"

struct Resource_System_State {
    Resource_System_Config config;
    Resource_Loader* registered_loaders; // Array of registered laoders
};

internal_var Resource_System_State state = {};

b8 resource_system_init(Resource_System_Config config) {
    state.config = config;
    u32 count = state.config.max_loader_count;

    state.registered_loaders = static_cast<Resource_Loader*>(
        memory_allocate(sizeof(Resource_Loader) * count, Memory_Tag::LOADER));

    for (u32 i = 0; i < count; ++i) {
        // Invalidate all loader IDs
        state.registered_loaders[i].id = INVALID_ID;
    }

    resource_system_register_loader(text_resource_loader_create());
    resource_system_register_loader(binary_resource_loader_create());
    resource_system_register_loader(image_resource_loader_create());
    resource_system_register_loader(icon_resource_loader_create());
    resource_system_register_loader(material_resource_loader_create());
    resource_system_register_loader(font_resource_loader_create());

    CORE_TRACE("Resource system initialized with base path '%s'",
        config.asset_base_path);

    return true;
}

void resource_system_shutdown() {
    memory_deallocate(state.registered_loaders,
        sizeof(Resource_Loader) * state.config.max_loader_count,
        Memory_Tag::LOADER);
}

VOLTRUM_API b8 resource_system_register_loader(Resource_Loader loader) {
    u32 count = state.config.max_loader_count;
    // First check whether this type has already a registered loader in the
    // registry
    for (u32 i = 0; i < count; ++i) {
        Resource_Loader* present_loader = &state.registered_loaders[i];
        if (present_loader->id != INVALID_ID) {
            if (present_loader->type == loader.type) {
                CORE_ERROR(
                    "resource_system_register_loader - Loader of type %d "
                    "already exists in the registry. Will be skipped.",
                    loader.type);

                return false;
            }
        }
    }

    for (u32 i = 0; i < count; ++i) {
        Resource_Loader* present_loader = &state.registered_loaders[i];
        if (present_loader->id == INVALID_ID) {
            *present_loader = loader;
            present_loader->id = i;
            CORE_TRACE("Loader registered.");
            return true;
        }
    }

    return false;
}

VOLTRUM_API b8 resource_system_load(const char* name,
    Resource_Type type,
    Resource* out_resource) {

    if (!name) {
        CORE_ERROR("resource_system_load - Invalid name provided. Returning.");
        return false;
    }

    if (!out_resource) {
        CORE_ERROR(
            "resource_system_load - Out resource object is nullptr. Returning");
        return false;
    }

    u32 count = state.config.max_loader_count;
    Resource_Loader* loader = nullptr;

    for (u32 i = 0; i < count; ++i) {
        Resource_Loader* present_loader = &state.registered_loaders[i];
        if (present_loader->id != INVALID_ID && present_loader->type == type) {
            loader = present_loader;
            break;
        }
    }

    if (!loader || !loader->load) {
        CORE_ERROR(
            "resource_system_loader - Failed to find loader for type %d. "
            "Skipping resource.",
            type);
        return false;
    }

    out_resource->loader_id = loader->id;
    return loader->load(loader, name, out_resource);
}

VOLTRUM_API void resource_system_unload(Resource* resource) {
    if (resource->loader_id != INVALID_ID) {
        Resource_Loader* loader =
            &state.registered_loaders[resource->loader_id];
        if (loader->id != INVALID_ID && loader->unload) {
            loader->unload(loader, resource);
        }
    }
}

VOLTRUM_API const char* resource_system_base_path() {
    return state.config.asset_base_path;
}
