#pragma once

#include "defines.hpp"

struct Allocator {
    // Optional user data passed to callbacks (e.g., arena pointer, pool, etc.)
    void* context;

    // Allocate a block of memory with the given alignment. Returns nullptr on failure.
    void* (*allocate)(void* context, u64 size, u64 alignment);

    // Free a block previously allocated by this allocator. Size can be used for statistics.
    void (*deallocate)(void* context, void* ptr, u64 size);

    // Optional: resize an existing allocation. Can be nullptr if not supported.
    void* (*reallocate)(void* context,
        void* ptr,
        u64 old_size,
        u64 new_size,
        u64 alignment);
};
