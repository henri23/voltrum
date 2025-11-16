#pragma once

#include "defines.hpp"
#include "memory/allocator.hpp"

// Memory Arena System
// Based on Ryan Fleury's arena allocator approach
// Provides fast linear allocation with simple deallocation patterns

struct Memory_Arena {
    u8* base;           // Base address of the arena
    u64 size;           // Total size of the arena
    u64 position;       // Current position (bytes allocated)
    u64 commit_position; // Position of committed memory (for virtual memory management)
};

// Arena creation and destruction
Memory_Arena* arena_create(u64 size);
Memory_Arena* arena_create_virtual(u64 reserve_size, u64 commit_size);
void arena_destroy(Memory_Arena* arena);

// Create an Allocator view over an existing arena.
// allocate/deallocate map to arena_alloc_aligned/arena_clear semantics.
Allocator arena_allocator(Memory_Arena* arena);

// Basic allocation
void* arena_alloc(Memory_Arena* arena, u64 size);
void* arena_alloc_zero(Memory_Arena* arena, u64 size);
void* arena_alloc_aligned(Memory_Arena* arena, u64 size, u64 alignment);

// Templated allocation for convenience (C++ style)
template<typename T>
T* arena_alloc(Memory_Arena* arena) {
    return (T*)arena_alloc_aligned(arena, sizeof(T), alignof(T));
}

template<typename T>
T* arena_alloc_array(Memory_Arena* arena, u64 count) {
    return (T*)arena_alloc_aligned(arena, sizeof(T) * count, alignof(T));
}

// Arena management
void arena_clear(Memory_Arena* arena);
void arena_reset_to_position(Memory_Arena* arena, u64 position);
u64 arena_get_position(Memory_Arena* arena);
u64 arena_get_remaining(Memory_Arena* arena);

// Arena state saving/restoring (for temporary allocations)
struct Arena_Checkpoint {
    u64 position;
};

Arena_Checkpoint arena_checkpoint(Memory_Arena* arena);
void arena_restore(Memory_Arena* arena, Arena_Checkpoint checkpoint);

// Scoped arena allocation (RAII-style)
struct Arena_Scope {
    Memory_Arena* arena;
    Arena_Checkpoint checkpoint;

    Arena_Scope(Memory_Arena* a) : arena(a), checkpoint(arena_checkpoint(a)) {}
    ~Arena_Scope() { arena_restore(arena, checkpoint); }
};

// Convenience macros
#define ARENA_SCOPE(arena) Arena_Scope _arena_scope(arena)
#define ARENA_ALLOC(arena, type) arena_alloc<type>(arena)
#define ARENA_ALLOC_ARRAY(arena, type, count) arena_alloc_array<type>(arena, count)

// Memory Pool System (for fixed-size allocations)

struct Memory_Pool_Block {
    Memory_Pool_Block* next;
};

struct Memory_Pool {
    Memory_Arena* arena;        // Arena that backs this pool
    Memory_Pool_Block* free_list; // List of free blocks
    u64 block_size;             // Size of each block
    u64 block_count;            // Number of blocks in the pool
    u64 blocks_allocated;       // Number of blocks currently allocated
};

// Pool creation and destruction
Memory_Pool* pool_create(Memory_Arena* backing_arena, u64 block_size, u64 block_count);
Memory_Pool* pool_create_standalone(u64 block_size, u64 block_count);
void pool_destroy(Memory_Pool* pool);

// Pool allocation/deallocation
void* pool_alloc(Memory_Pool* pool);
void pool_free(Memory_Pool* pool, void* ptr);
void pool_clear(Memory_Pool* pool);

// Pool statistics
u64 pool_get_blocks_allocated(Memory_Pool* pool);
u64 pool_get_blocks_free(Memory_Pool* pool);
f32 pool_get_utilization(Memory_Pool* pool); // Returns 0.0 to 1.0

// Templated pool allocation
template<typename T>
T* pool_alloc(Memory_Pool* pool) {
    return (T*)pool_alloc(pool);
}

// Global arena management (optional convenience)
Memory_Arena* get_frame_arena();     // Per-frame temporary allocations
Memory_Arena* get_scratch_arena();   // General scratch allocations
Memory_Arena* get_persistent_arena(); // Long-lived allocations

void init_global_arenas();
void shutdown_global_arenas();
void clear_frame_arena(); // Call this each frame
