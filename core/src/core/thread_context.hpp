#pragma once

#include "defines.hpp"
#include "memory/arena.hpp"

struct Thread_Context {
    Arena *arenas[2];
    u32 thread_id;
    const char *thread_name;
};

Thread_Context *thread_context_allocate();

void thread_context_release(Thread_Context *context);

void thread_context_select(Thread_Context *context);

INTERNAL_FUNC Thread_Context *thread_context_selected();

// Returns a scratch arena that is not being used in an alternating fashion
INTERNAL_FUNC Arena *thread_context_get_scratch(Arena **conflicts, u64 count);

// If the function that is calling scratch begin has an arena pointer that is
// being supplied by its caller, it means that the called function has data to
// persist. In such case that arena pointer should be passed as conflicts here
// so that it is not used as a scratch arena
#define scratch_begin(conflicts, count)                                        \
        arena_scratch_begin(thread_context_get_scratch((conflicts), (count))

#define scratch_end(scratch) arena_scratch_end(scratch)
