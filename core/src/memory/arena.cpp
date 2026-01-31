#include "arena.hpp"

#include "core/asserts.hpp"
#include "core/logger.hpp"
#include "platform/platform.hpp"

#if ASAN_ENABLED
C_LINKAGE const char *
__lsan_default_suppressions()
{
    return "leak:libnvidia-glcore.so\n"
           "leak:libdbus-1.so\n"
           "leak:Vulkan-ValidationLayers\n"
           "leak:Vulkan-Loader\n";
}
#endif

Arena *
_arena_create(const char *file, s32 line, u64 reserve_size, u64 commit_size) {

    auto sys_info = platform_query_system_info();

    u64 aligned_reserve_size = ALIGN_UP_POW2(reserve_size, sys_info.page_size);
    u64 aligned_commit_size = ALIGN_UP_POW2(commit_size, sys_info.page_size);

    void *block = platform_virtual_memory_reserve(aligned_reserve_size);
    platform_virtual_memory_commit(block, aligned_commit_size);

    CORE_DEBUG("Arena allocated at %s:%i", file, line);

    // Cold cast the arena header point at the start of the allocated block
    Arena *arena = (Arena *)block;
    arena->memory = block;
    arena->committed_memory = aligned_commit_size;
    arena->commit_granularity = aligned_commit_size;
    arena->reserved_memory = aligned_reserve_size;
    arena->offset = ARENA_HEADER_SIZE;
    arena->allocation_file = file;
    arena->allocation_line = line;

    // Poison all committed memory past the header. This marks the region as
    // "off-limits" so ASAN can detect out-of-bounds accesses within the arena.
    // The header itself (0..ARENA_HEADER_SIZE) stays unpoisoned since it's
    // actively used by the Arena struct.
    ASAN_POISON_MEMORY_REGION(
        static_cast<u8 *>(block) + ARENA_HEADER_SIZE,
        aligned_commit_size - ARENA_HEADER_SIZE);

    return arena;
}

void
arena_release(Arena *arena) {
    // Unpoison everything before releasing so ASAN doesn't complain about
    // the platform layer touching poisoned memory during decommit/release
    ASAN_UNPOISON_MEMORY_REGION(arena->memory, arena->committed_memory);
    platform_virtual_memory_release(arena->memory, arena->reserved_memory);
}

void *
_arena_push(Arena *arena, u64 size, u64 align, b8 should_zero) {
    // Align should always be a power of 2
    // I.E:	1	->	char
    // 		2 	->	short
    // 		4 	->	int, float
    // 		8 	->	long, double
    // 		16 	->	SSE
    // 		32 	->	AVX
    u64 current_offset = ALIGN_UP_POW2(arena->offset, align);
    u64 requested_offset = current_offset + size;

    RUNTIME_ASSERT_MSG(requested_offset <= arena->reserved_memory,
        "Arena allocated at %s:%i exceeds reserved memory limit");

    // Compute size to be zeroed before commtting the new pages. We consider the
    // memory from the current_offset until the next already commited page. In
    // case there will be a new page allocated, it is ensured to be 0 by the OS
    u64 size_to_zero = 0;
    if (should_zero) {
        size_to_zero =
            MIN(arena->committed_memory, requested_offset) - current_offset;
    }

    // Check if new pages need to be committed
    if (arena->committed_memory < requested_offset) {
        // Since the requested offset might not necessarily be a power of 2 we
        // need to round up to an integer multiple of the page size that can
        // fit the requested_offset manually (cannot use ALIGN_UP_POW2)
        u64 aligned_requested_commit_offset =
            ALIGN_UP(requested_offset, arena->commit_granularity);

        RUNTIME_ASSERT_MSG(aligned_requested_commit_offset <=
                               arena->reserved_memory,
            "_arena_push - Arena exceeds reserved memory");

        u64 commit_size =
            aligned_requested_commit_offset - arena->committed_memory;

        u8 *commit_pointer =
            static_cast<u8 *>(arena->memory) + arena->committed_memory;

        platform_virtual_memory_commit(commit_pointer, commit_size);

        // Poison the newly committed pages — they are not yet in use.
        // The portion that covers the current allocation will be unpoisoned
        // below when we unpoison the result pointer.
        ASAN_POISON_MEMORY_REGION(commit_pointer, commit_size);

        arena->committed_memory = aligned_requested_commit_offset;
    }

    RUNTIME_ASSERT_MSG(arena->committed_memory >= requested_offset,
        "_arena_push - Committed memory does not cover the requested "
        "allocation");

    void *result = static_cast<u8 *>(arena->memory) + current_offset;

    // Unpoison the region being handed out so ASAN allows access to it
    ASAN_UNPOISON_MEMORY_REGION(result, size);

    arena->offset = requested_offset; // Update offset

    if (should_zero)
        platform_zero_memory(result, size_to_zero);

    return result;
}

void
arena_pop_to(Arena *arena, u64 position) {
    u64 new_position = CLAMP_BOT(ARENA_HEADER_SIZE, position);

    RUNTIME_ASSERT_MSG(new_position <= arena->offset,
        "arena_pop_to - Cannot pop to a positon that is ahead of the current "
        "position");

    // Re-poison the region being freed so ASAN catches use-after-pop accesses
    if (new_position < arena->offset)
    {
        ASAN_POISON_MEMORY_REGION(
            static_cast<u8 *>(arena->memory) + new_position,
            arena->offset - new_position);
    }

    arena->offset = new_position;
}

void
arena_pop(Arena *arena, u64 size) {
    u64 new_position = arena->offset - size;

    arena_pop_to(arena, new_position);
}

void
arena_clear(Arena *arena) {
    arena_pop_to(arena, 0); // Specify 0 as new position because it will be
                            // clamped to ARENA_HEADER_SIZE
}

Scratch_Arena
arena_scratch_begin(Arena *arena) {
    u64 position = arena->offset;

    Scratch_Arena scratch{arena, position};

    return scratch;
}

void
arena_scratch_end(Scratch_Arena scratch) {
    arena_pop_to(scratch.arena, scratch.position);
}
