#include "memory_arena.hpp"
#include "memory.hpp"
#include "core/logger.hpp"
#include "core/asserts.hpp"
#include "platform/platform.hpp"
#include "memory/allocator.hpp"
#include <cstddef>

// ===== Memory Arena Implementation =====

Memory_Arena* arena_create(u64 size) {
    RUNTIME_ASSERT_MSG(size > 0, "Arena size must be greater than 0");

    // Allocate the arena struct itself
    Memory_Arena* arena = (Memory_Arena*)memory_allocate(sizeof(Memory_Arena), Memory_Tag::LINEAR_ALLOCATOR);
    if (!arena) {
        CORE_ERROR("Failed to allocate arena structure");
        return nullptr;
    }

    // Allocate the arena memory
    arena->base = (u8*)memory_allocate(size, Memory_Tag::LINEAR_ALLOCATOR);
    if (!arena->base) {
        CORE_ERROR("Failed to allocate arena memory of size %llu", size);
        memory_deallocate(arena, sizeof(Memory_Arena), Memory_Tag::LINEAR_ALLOCATOR);
        return nullptr;
    }

    arena->size = size;
    arena->position = 0;
    arena->commit_position = size; // For regular allocation, everything is committed

    CORE_DEBUG("Created arena: %llu bytes at %p", size, arena->base);
    return arena;
}

Memory_Arena* arena_create_virtual(u64 reserve_size, u64 commit_size) {
    RUNTIME_ASSERT_MSG(reserve_size > 0, "Reserve size must be greater than 0");
    RUNTIME_ASSERT_MSG(commit_size <= reserve_size, "Commit size cannot exceed reserve size");

    // For now, we'll implement this as a regular arena
    // In a full implementation, this would use virtual memory APIs
    CORE_WARN("Virtual arena creation not fully implemented, falling back to regular arena");
    return arena_create(commit_size);
}

void arena_destroy(Memory_Arena* arena) {
    if (!arena) {
        return;
    }

    if (arena->base) {
        memory_deallocate(arena->base, arena->size, Memory_Tag::LINEAR_ALLOCATOR);
    }

    memory_deallocate(arena, sizeof(Memory_Arena), Memory_Tag::LINEAR_ALLOCATOR);
}

void* arena_alloc(Memory_Arena* arena, u64 size) {
    RUNTIME_ASSERT_MSG(arena, "Arena cannot be null");
    RUNTIME_ASSERT_MSG(size > 0, "Allocation size must be greater than 0");

    if (arena->position + size > arena->size) {
        CORE_ERROR("Arena out of memory: requested %llu bytes, available %llu bytes",
                   size, arena->size - arena->position);
        return nullptr;
    }

    void* result = arena->base + arena->position;
    arena->position += size;

    return result;
}

void* arena_alloc_zero(Memory_Arena* arena, u64 size) {
    void* result = arena_alloc(arena, size);
    if (result) {
        memory_zero(result, size);
    }
    return result;
}

void* arena_alloc_aligned(Memory_Arena* arena, u64 size, u64 alignment) {
    RUNTIME_ASSERT_MSG(arena, "Arena cannot be null");
    RUNTIME_ASSERT_MSG(size > 0, "Allocation size must be greater than 0");
    RUNTIME_ASSERT_MSG(alignment > 0 && (alignment & (alignment - 1)) == 0,
                      "Alignment must be a power of 2");

    // Calculate aligned position
    u64 current_pos = arena->position;
    u64 aligned_pos = (current_pos + alignment - 1) & ~(alignment - 1);
    u64 aligned_size = aligned_pos - current_pos + size;

    if (aligned_pos + size > arena->size) {
        CORE_ERROR("Arena out of memory for aligned allocation: requested %llu bytes (aligned), available %llu bytes",
                   aligned_size, arena->size - current_pos);
        return nullptr;
    }

    arena->position = aligned_pos + size;
    return arena->base + aligned_pos;
}

// -------- Allocator adapter --------
internal_variable void* arena_allocator_allocate(void* context, u64 size, u64 alignment) {
    Memory_Arena* arena = static_cast<Memory_Arena*>(context);
    return arena_alloc_aligned(arena, size, alignment ? alignment : alignof(std::max_align_t));
}

internal_variable void arena_allocator_deallocate(void* context, void* ptr, u64 size) {
    (void)context;
    (void)ptr;
    (void)size;
    // Linear arenas do not support individual frees; user controls lifetime via checkpoints/clear.
}

internal_variable void* arena_allocator_reallocate(void* context,
    void* ptr,
    u64 old_size,
    u64 new_size,
    u64 alignment) {
    (void)ptr;
    (void)old_size;
    // Simplest policy: allocate new block; caller copies if needed.
    return arena_allocator_allocate(context, new_size, alignment);
}

Allocator arena_allocator(Memory_Arena* arena) {
    Allocator allocator = {};
    allocator.context = arena;
    allocator.allocate = arena_allocator_allocate;
    allocator.deallocate = arena_allocator_deallocate;
    allocator.reallocate = arena_allocator_reallocate;
    return allocator;
}

void arena_clear(Memory_Arena* arena) {
    RUNTIME_ASSERT_MSG(arena, "Arena cannot be null");
    arena->position = 0;
}

void arena_reset_to_position(Memory_Arena* arena, u64 position) {
    RUNTIME_ASSERT_MSG(arena, "Arena cannot be null");
    RUNTIME_ASSERT_MSG(position <= arena->size, "Position cannot exceed arena size");
    arena->position = position;
}

u64 arena_get_position(Memory_Arena* arena) {
    RUNTIME_ASSERT_MSG(arena, "Arena cannot be null");
    return arena->position;
}

u64 arena_get_remaining(Memory_Arena* arena) {
    RUNTIME_ASSERT_MSG(arena, "Arena cannot be null");
    return arena->size - arena->position;
}

Arena_Checkpoint arena_checkpoint(Memory_Arena* arena) {
    RUNTIME_ASSERT_MSG(arena, "Arena cannot be null");
    Arena_Checkpoint checkpoint;
    checkpoint.position = arena->position;
    return checkpoint;
}

void arena_restore(Memory_Arena* arena, Arena_Checkpoint checkpoint) {
    RUNTIME_ASSERT_MSG(arena, "Arena cannot be null");
    arena_reset_to_position(arena, checkpoint.position);
}

// ===== Memory Pool Implementation =====

Memory_Pool* pool_create(Memory_Arena* backing_arena, u64 block_size, u64 block_count) {
    RUNTIME_ASSERT_MSG(backing_arena, "Backing arena cannot be null");
    RUNTIME_ASSERT_MSG(block_size > 0, "Block size must be greater than 0");
    RUNTIME_ASSERT_MSG(block_count > 0, "Block count must be greater than 0");

    // Ensure block size is at least as large as a pointer (for free list)
    u64 min_block_size = sizeof(Memory_Pool_Block*);
    if (block_size < min_block_size) {
        block_size = min_block_size;
    }

    // Allocate pool structure from the backing arena
    Memory_Pool* pool = arena_alloc<Memory_Pool>(backing_arena);
    if (!pool) {
        CORE_ERROR("Failed to allocate pool structure");
        return nullptr;
    }

    // Calculate total memory needed for all blocks
    u64 total_memory = block_size * block_count;

    // Allocate memory for all blocks from the backing arena
    u8* block_memory = (u8*)arena_alloc(backing_arena, total_memory);
    if (!block_memory) {
        CORE_ERROR("Failed to allocate pool memory");
        return nullptr;
    }

    // Initialize pool
    pool->arena = backing_arena;
    pool->free_list = nullptr;
    pool->block_size = block_size;
    pool->block_count = block_count;
    pool->blocks_allocated = 0;

    // Initialize free list - link all blocks together
    for (u64 i = 0; i < block_count; ++i) {
        Memory_Pool_Block* block = (Memory_Pool_Block*)(block_memory + i * block_size);
        block->next = pool->free_list;
        pool->free_list = block;
    }

    CORE_DEBUG("Created pool: %llu blocks of %llu bytes each", block_count, block_size);
    return pool;
}

Memory_Pool* pool_create_standalone(u64 block_size, u64 block_count) {
    // Create a backing arena for the pool
    u64 arena_size = sizeof(Memory_Pool) + (block_size * block_count) + 64; // Extra for alignment
    Memory_Arena* arena = arena_create(arena_size);
    if (!arena) {
        return nullptr;
    }

    return pool_create(arena, block_size, block_count);
}

void pool_destroy(Memory_Pool* pool) {
    // Note: If the pool was created standalone, the backing arena should be destroyed
    // For now, we don't track this automatically
    // In a full implementation, we'd need to track arena ownership
    (void)pool; // Pool memory is owned by the backing arena
}

void* pool_alloc(Memory_Pool* pool) {
    RUNTIME_ASSERT_MSG(pool, "Pool cannot be null");

    if (!pool->free_list) {
        CORE_WARN("Pool exhausted: no free blocks available");
        return nullptr;
    }

    // Take a block from the free list
    Memory_Pool_Block* block = pool->free_list;
    pool->free_list = block->next;
    pool->blocks_allocated++;

    // Zero the block memory (optional, but helpful for debugging)
    memory_zero(block, pool->block_size);

    return block;
}

void pool_free(Memory_Pool* pool, void* ptr) {
    RUNTIME_ASSERT_MSG(pool, "Pool cannot be null");
    RUNTIME_ASSERT_MSG(ptr, "Pointer cannot be null");

    // Add the block back to the free list
    Memory_Pool_Block* block = (Memory_Pool_Block*)ptr;
    block->next = pool->free_list;
    pool->free_list = block;
    pool->blocks_allocated--;
}

void pool_clear(Memory_Pool* pool) {
    RUNTIME_ASSERT_MSG(pool, "Pool cannot be null");

    // This is tricky - we need to rebuild the free list
    // For now, we'll just reset the allocation counter and warn
    CORE_WARN("Pool clear not fully implemented - memory may be leaked until pool destruction");
    pool->blocks_allocated = 0;
}

u64 pool_get_blocks_allocated(Memory_Pool* pool) {
    RUNTIME_ASSERT_MSG(pool, "Pool cannot be null");
    return pool->blocks_allocated;
}

u64 pool_get_blocks_free(Memory_Pool* pool) {
    RUNTIME_ASSERT_MSG(pool, "Pool cannot be null");
    return pool->block_count - pool->blocks_allocated;
}

f32 pool_get_utilization(Memory_Pool* pool) {
    RUNTIME_ASSERT_MSG(pool, "Pool cannot be null");
    return (f32)pool->blocks_allocated / (f32)pool->block_count;
}

// ===== Global Arena Management =====

static Memory_Arena* g_frame_arena = nullptr;
static Memory_Arena* g_scratch_arena = nullptr;
static Memory_Arena* g_persistent_arena = nullptr;

Memory_Arena* get_frame_arena() {
    return g_frame_arena;
}

Memory_Arena* get_scratch_arena() {
    return g_scratch_arena;
}

Memory_Arena* get_persistent_arena() {
    return g_persistent_arena;
}

void init_global_arenas() {
    CORE_DEBUG("Initializing global memory arenas...");

    // Create global arenas with reasonable default sizes
    g_frame_arena = arena_create(1 * 1024 * 1024);      // 1 MB for per-frame allocations
    g_scratch_arena = arena_create(4 * 1024 * 1024);    // 4 MB for general scratch use
    g_persistent_arena = arena_create(16 * 1024 * 1024); // 16 MB for persistent allocations

    if (!g_frame_arena || !g_scratch_arena || !g_persistent_arena) {
        CORE_ERROR("Failed to initialize global arenas");
        return;
    }

    CORE_INFO("Global memory arenas initialized successfully");
}

void shutdown_global_arenas() {
    CORE_DEBUG("Shutting down global memory arenas...");

    if (g_frame_arena) {
        arena_destroy(g_frame_arena);
        g_frame_arena = nullptr;
    }

    if (g_scratch_arena) {
        arena_destroy(g_scratch_arena);
        g_scratch_arena = nullptr;
    }

    if (g_persistent_arena) {
        arena_destroy(g_persistent_arena);
        g_persistent_arena = nullptr;
    }

    CORE_DEBUG("Global memory arenas shut down successfully");
}

void clear_frame_arena() {
    if (g_frame_arena) {
        arena_clear(g_frame_arena);
    }
}
