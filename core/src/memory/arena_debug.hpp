#pragma once

#ifdef DEBUG_BUILD

#    include "defines.hpp"

struct Arena;

struct Arena_Allocation_Record
{
    u64         offset;  // Start offset of the allocation
    u64         size;    // Requested size
    u64         padding; // Alignment gap before this allocation
    const char *file;    // Source file
    s32         line;    // Source line
};

constexpr u32 ARENA_DEBUG_MAX_ARENAS         = 64;
constexpr u32 ARENA_DEBUG_INITIAL_RECORD_CAP = 256;

struct Arena_Debug_Entry
{
    Arena                   *arena;
    b8                       active;
    Arena_Allocation_Record *records;
    u32                      record_count;
    u32                      record_capacity;
};

struct Arena_Debug_Registry
{
    Arena_Debug_Entry entries[ARENA_DEBUG_MAX_ARENAS];
    u32               active_count;
};

VOLTRUM_API void arena_debug_init();
VOLTRUM_API void arena_debug_shutdown();

void arena_debug_register(Arena *arena);
void arena_debug_deregister(Arena *arena);

void arena_debug_record_push(Arena      *arena,
                             u64         offset,
                             u64         size,
                             u64         padding,
                             const char *file,
                             s32         line);

void arena_debug_record_pop_to(Arena *arena, u64 new_position);

VOLTRUM_API Arena_Debug_Registry *arena_debug_get_registry();

#endif // DEBUG_BUILD
