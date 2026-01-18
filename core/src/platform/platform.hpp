#pragma once

#include "defines.hpp"

#include <SDL3/SDL.h>

struct Platform_State {
    SDL_Window *window;
    SDL_Renderer *renderer;
    f32 main_scale;
    b8 is_titlebar_hovered;
};

b8 platform_startup(Platform_State *state,
    const char *application_name,
    s32 width,
    s32 height);

void platform_shutdown();

b8 platform_message_pump();

// Platform specific memory management
void *platform_allocate(u64 size, b8 aligned);
void platform_free(void *block, b8 aligned);
void *platform_zero_memory(void *block, u64 size);
void *platform_copy_memory(void *dest, const void *source, u64 size);
void *platform_move_memory(void *dest, const void *source, u64 size);
void *platform_set_memory(void *dest, s32 value, u64 size);

// Virtual memory - OS lazily backs with physical pages on first access.
// Pages are guaranteed to be zero-initialized when first accessed.
void *platform_virtual_memory_reserve(u64 size);
void *platform_virtual_memory_commit(void *memory);
void platform_virtual_memory_release(void *block);
u64 platform_query_page_size();

f64 platform_get_absolute_time();
void platform_sleep(u64 ms);

b8 platform_get_drawable_size(u32 *width, u32 *height);

// Window control functions
b8 platform_get_window_details(u32 *width, u32 *height, f32 *main_scale);
void platform_minimize_window();
void platform_maximize_window();
void platform_restore_window();

VOLTRUM_API void platform_close_window();
b8 platform_is_window_maximized();

void platform_set_window_icon(u8 *pixels, u32 width, u32 height);
void platform_set_window_position(s32 x, s32 y);
void platform_get_window_position(s32 *x, s32 *y);
void platform_set_window_size(s32 width, s32 height);
void platform_get_window_size(s32 *width, s32 *height);
void platform_set_titlebar_hovered(b8 hovered);
