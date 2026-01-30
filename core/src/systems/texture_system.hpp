#pragma once

#include "data_structures/hashmap.hpp"
#include "defines.hpp"
#include "resources/resource_types.hpp"

struct Texture_System_Config
{
    u32 max_texture_count;
};

struct Texture_Reference
{
    Texture_ID handle;
    u64        reference_count;
    b8         auto_release;
};

struct Texture_System_State
{
    Texture_System_Config config;
    Texture               default_texture;

    Hashmap<Texture_Reference> texture_registry;
    Texture                   *registered_textures;
};

#define DEFAULT_TEXTURE_NAME "default_"

Texture_System_State *texture_system_init(Arena                *allocator,
                                          Texture_System_Config config);

void     texture_system_shutdown();
Texture *texture_system_acquire(const char *name,
                                b8          auto_release  = true,
                                b8          is_ui_texture = false);
void     texture_system_release(const char *name);
Texture *texture_system_get_default_texture();
