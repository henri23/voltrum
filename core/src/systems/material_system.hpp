#pragma once

#include "data_structures/hashmap.hpp"
#include "defines.hpp"
#include "resources/resource_types.hpp"

#define DEFAULT_MATERIAL_NAME "default_"

struct Material_System_Config
{
    u32 max_material_count;
};

struct Material_Reference
{
    Material_ID handle;
    u64         reference_count;
    b8          auto_release;
};

struct Material_System_State
{
    Material_System_Config config;
    Material               default_material;

    Hashmap<Material_Reference> material_registry;
    Material                   *registered_materials;
};

Material_System_State *material_system_init(Arena                 *allocator,
                                            Material_System_Config config);

void material_system_shutdown();

Material *material_system_acquire(const char *name);

Material *material_system_acquire_from_config(Material_Config config);

void material_system_release(const char *name);

Material *material_system_get_default();
