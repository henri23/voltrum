#pragma once

#include "defines.hpp"

enum class Memory_Tag {
    UNKNOWN,
    ARRAY,
    DARRAY,
    HASHMAP,
    LINEAR_ALLOCATOR,
    EVENTS,
    STRING,
    CLIENT,
    INPUT,
    RENDERER,
    TEXTURE,
    MATERIAL,
    GEOMETRY,
    APPLICATION,
    UI,
    LAYERS,
    MAX_ENTRIES
};

void memory_init();

void memory_shutdown();

VOLTRUM_API void* memory_allocate(u64 size, Memory_Tag tag);

VOLTRUM_API void* memory_allocate(u64 size, Memory_Tag tag);

VOLTRUM_API void memory_deallocate(void* block, u64 size, Memory_Tag tag);

VOLTRUM_API void* memory_zero(void* block, u64 size);

VOLTRUM_API void* memory_copy(void* destination, const void* source, u64 size);

VOLTRUM_API void* memory_move(void* destination, const void* source, u64 size);

VOLTRUM_API void* memory_set(void* block, s32 value, u64 size);

VOLTRUM_API void memory_get_current_usage(char* out_buf);

VOLTRUM_API u64 memory_get_allocations_count();
