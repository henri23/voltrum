#pragma once

#include "defines.hpp"
#include "resources/resource_types.hpp"

#define DEFAULT_MATERIAL_NAME "default_"

struct Material_System_Config {
    u32 max_material_count;
};

struct Material_Config {
    char name[MATERIAL_NAME_MAX_LENGTH];
    b8 auto_release;
    vec4 diffuse_color;
    char diffuse_map_name[TEXTURE_NAME_MAX_LENGTH];
};

b8 material_system_init(Material_System_Config config);
void material_system_shutdown();

Material* material_system_acquire(const char* name);

Material* material_system_acquire_from_config(Material_Config config);

void material_system_release(const char* name);

Material* material_system_get_default();
