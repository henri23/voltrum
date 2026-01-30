#include "resource_system.hpp"
#include "core/asserts.hpp"
#include "core/logger.hpp"

// List of system resource loaders that are known to the core library
#include "resources/loaders/binary_loader.hpp"
#include "resources/loaders/font_loader.hpp"
#include "resources/loaders/icon_loader.hpp"
#include "resources/loaders/image_loader.hpp"
#include "resources/loaders/material_loader.hpp"
#include "resources/loaders/text_loader.hpp"

internal_var Resource_System_State *state_ptr = nullptr;

Resource_System_State *
resource_system_init(Arena *arena, Resource_System_Config config)
{

    auto *state = push_struct(arena, Resource_System_State);

    state->config = config;

    state->registered_loaders =
        push_array(arena, Resource_Loader, config.max_loader_count);

    for (u32 i = 0; i < config.max_loader_count; ++i)
    {
        // Invalidate all loader IDs
        state->registered_loaders[i].id = INVALID_ID;
    }

    resource_system_register_loader(state, text_resource_loader_create());
    resource_system_register_loader(state, binary_resource_loader_create());
    resource_system_register_loader(state, image_resource_loader_create());
    resource_system_register_loader(state, icon_resource_loader_create());
    resource_system_register_loader(state, material_resource_loader_create());
    resource_system_register_loader(state, font_resource_loader_create());

    CORE_TRACE("Resource system initialized with base path '%s'",
               config.asset_base_path);

    state_ptr = state;

    return state;
}

VOLTRUM_API b8
resource_system_register_loader(Resource_System_State *state,
                                Resource_Loader        loader)
{

    u32 count = state->config.max_loader_count;

    // First check whether this type has already a registered loader in the
    // registry
    for (u32 i = 0; i < count; ++i)
    {
        Resource_Loader *present_loader = &state->registered_loaders[i];
        if (present_loader->id != INVALID_ID)
        {
            if (present_loader->type == loader.type)
            {
                CORE_ERROR(
                    "resource_system_register_loader - Loader of type %d "
                    "already exists in the registry. Will be skipped.",
                    loader.type);

                return false;
            }
        }
    }

    for (u32 i = 0; i < count; ++i)
    {
        Resource_Loader *present_loader = &state->registered_loaders[i];
        if (present_loader->id == INVALID_ID)
        {
            *present_loader    = loader;
            present_loader->id = i;
            CORE_TRACE("Loader registered.");
            return true;
        }
    }

    return false;
}

VOLTRUM_API b8
resource_system_load(const char   *name,
                     Resource_Type type,
                     Resource     *out_resource)
{
    ENSURE(state_ptr);

    if (!name)
    {
        CORE_ERROR("resource_system_load - Invalid name provided. Returning.");
        return false;
    }

    if (!out_resource)
    {
        CORE_ERROR(
            "resource_system_load - Out resource object is nullptr. Returning");
        return false;
    }

    u32              count  = state_ptr->config.max_loader_count;
    Resource_Loader *loader = nullptr;

    for (u32 i = 0; i < count; ++i)
    {
        Resource_Loader *present_loader = &state_ptr->registered_loaders[i];
        if (present_loader->id != INVALID_ID && present_loader->type == type)
        {
            loader = present_loader;
            break;
        }
    }

    if (!loader || !loader->load)
    {
        CORE_ERROR(
            "resource_system_loader - Failed to find loader for type %d. "
            "Skipping resource.",
            type);
        return false;
    }

    out_resource->loader_id = loader->id;
    return loader->load(loader, name, out_resource);
}

VOLTRUM_API void
resource_system_unload(Resource *resource)
{
    ENSURE(state_ptr);

    if (resource->loader_id != INVALID_ID)
    {
        Resource_Loader *loader =
            &state_ptr->registered_loaders[resource->loader_id];
        if (loader->id != INVALID_ID && loader->unload)
        {
            loader->unload(loader, resource);
        }
    }
}

VOLTRUM_API const char *
resource_system_base_path()
{
    ENSURE(state_ptr);

    return state_ptr->config.asset_base_path;
}
