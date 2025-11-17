#pragma once

#include "defines.hpp"
#include "resources/resource_types.hpp"

struct Texture_System_Config {
    // Extend with fields such as cache sizes or maximum textures.
    u32 max_texture_count;
};

#define DEFAULT_TEXTURE_NAME "default_"

b8 texture_system_init(Texture_System_Config config);
void texture_system_shutdown();

Texture* texture_system_acquire(const char* name, b8 auto_release);
void texture_system_release(const char* name);

Texture* texture_system_get_default_texture();
