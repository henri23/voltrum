#pragma once

#include "defines.hpp"
#include "memory/arena.hpp"
#include "utils/string.hpp"

#include <SDL3/SDL.h>

struct Platform_State
{
    SDL_Window   *window;
    SDL_Renderer *renderer;
    f32           main_scale;

    // Set by the UI layer each frame to prevent OS titlebar drag
    // when an ImGui window overlaps the titlebar area
    b8 block_titlebar_drag;
};

struct Platform_System_Info
{
    u32         logical_processor_count;
    u64         page_size;
    u64         large_page_size;
    u64         allocation_granularity;
    const char *machine_name;
};

struct Platform_Process_Info
{
    u32         pid;
    b8          large_pages_allowed;
    const char *binary_path;
    const char *initial_path;
    const char *user_program_data_path;
};

Platform_State *
platform_init(Arena *allocator, String application_name, s32 width, s32 height);

void platform_shutdown(Platform_State *state);

b8 platform_message_pump(struct Frame_Context *frame_ctx);

// Platform specific memory management
// Virtual memory - OS lazily backs with physical pages on first access.
// Pages are guaranteed to be zero-initialized when first accessed.
void *platform_allocate(u64 size, b8 aligned);
void  platform_free(void *block, b8 aligned);
void *platform_zero_memory(void *block, u64 size);
void *platform_copy_memory(void *dest, const void *source, u64 size);
void *platform_move_memory(void *dest, const void *source, u64 size);
void *platform_set_memory(void *dest, s32 value, u64 size);
void *platform_virtual_memory_reserve(u64 size);
void  platform_virtual_memory_release(void *block, u64 size);
b8    platform_virtual_memory_commit(void *block, u64 size);
void  platform_virtual_memory_decommit(void *block, u64 size);

Platform_System_Info platform_query_system_info();
f64                  platform_get_absolute_time();
void                 platform_sleep(u64 ms);
void                 platform_get_drawable_size(u32 *width, u32 *height);

// Window control functions
VOLTRUM_API void platform_minimize_window(Platform_State *state);
VOLTRUM_API void platform_maximize_window(Platform_State *state);
VOLTRUM_API void platform_restore_window(Platform_State *state);
VOLTRUM_API void platform_close_window();
VOLTRUM_API b8   platform_is_window_maximized(Platform_State *state);

void platform_set_window_icon(Platform_State *state,
                              u8             *pixels,
                              u32             width,
                              u32             height);
