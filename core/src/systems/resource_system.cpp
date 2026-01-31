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

    CORE_TRACE("Resource system initialized with base path '%s'",
               config.asset_base_path);

    state_ptr = state;

    return state;
}

VOLTRUM_API b8
resource_system_load(Arena         *arena,
                     const char    *name,
                     Resource_Type  type,
                     Resource      *out_resource)
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

    switch (type)
    {
        case Resource_Type::TEXT:
            return text_loader_load(arena, name, out_resource);
        case Resource_Type::BINARY:
            return binary_loader_load(arena, name, out_resource);
        case Resource_Type::IMAGE:
            return image_loader_load(arena, name, out_resource);
        case Resource_Type::ICON:
            return icon_loader_load(arena, name, out_resource);
        case Resource_Type::MATERIAL:
            return material_loader_load(arena, name, out_resource);
        case Resource_Type::FONT:
            return font_loader_load(arena, name, out_resource);
        default:
            CORE_ERROR(
                "resource_system_load - Unknown resource type %d", type);
            return false;
    }
}

VOLTRUM_API const char *
resource_system_base_path()
{
    ENSURE(state_ptr);

    return state_ptr->config.asset_base_path;
}
