#pragma once

#include "resources/resource_types.hpp"

struct Resource_System_Config {
    u32 max_loader_count;
    const char* asset_base_path; // Relative base path for assets
};

struct Resource_Loader {
    Loader_ID id;
    Resource_Type type;
    const char* type_path;

    b8 (*load)(struct Resource_Loader* self,
        const char* name,
        Resource* out_resource);

    void (*unload)(struct Resource_Loader* self, Resource* resource);
};

b8 resource_system_init(Resource_System_Config config);
void resource_system_shutdown();

VOLTRUM_API b8 resource_system_register_loader(Resource_Loader loader);

VOLTRUM_API b8 resource_system_load(const char* name,
    Resource_Type type,
    Resource* out_resource);

VOLTRUM_API void resource_system_unload(Resource* resource);

VOLTRUM_API const char* resource_system_base_path();
