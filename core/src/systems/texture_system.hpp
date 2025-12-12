#pragma once

#include "defines.hpp"
#include "resources/resource_types.hpp"

struct Texture_System_Config {
    u32 max_texture_count;
};

#define DEFAULT_TEXTURE_NAME "default_"

b8 texture_system_init(Texture_System_Config config);
void texture_system_shutdown();

Texture* texture_system_acquire(
    const char* name,
    b8 auto_release = true,
    b8 is_ui_texture = false
);
void texture_system_release(const char* name);

Texture* texture_system_get_default_texture();
