#pragma once

#include "memory/arena.hpp"
#include "resources/resource_types.hpp"

struct Resource_System_Config
{
    const char *asset_base_path;
};

struct Resource_System_State
{
    Resource_System_Config config;
};

Resource_System_State *resource_system_init(Arena      *arena,
                                            Resource_System_Config config);

VOLTRUM_API b8 resource_system_load(Arena         *arena,
                                    const char    *name,
                                    Resource_Type  type,
                                    Resource      *out_resource);

VOLTRUM_API const char *resource_system_base_path();
