#include "thread_context.hpp"
#include "core/logger.hpp"

THREAD_STATIC Thread_Context *thread_local_context;

Thread_Context *
thread_context_allocate()
{
    Arena *arena = arena_create();

    auto *context = push_struct(arena, Thread_Context);

    context->arenas[0] = arena;
    context->arenas[1] = arena_create();

    return context;
}

void
thread_context_release(Thread_Context *context)
{
    // Save arenas[1] before releasing arenas[0], because
    // context itself lives inside arenas[0]
    Arena *arena1 = context->arenas[1];
    arena_release(context->arenas[0]);
    arena_release(arena1);
}

void
thread_context_select(Thread_Context *context)
{
    CORE_INFO("Thread '%s' spawned. Context selected.", context->thread_name);
    thread_local_context = context;
}

Thread_Context *
thread_context_selected()
{
    return thread_local_context;
}

Arena *
thread_context_get_scratch(Arena **conflicting_arenas, u64 conflict_count)
{
    Thread_Context *context = thread_context_selected();

    Arena **arena_ptr = context->arenas;

    for (u32 i = 0; i < ARRAY_COUNT(context->arenas); ++i)
    {
        Arena **conflict_ptr = conflicting_arenas;

        b8 has_conflict = false;

        // Scan each of the thread local arenas until we find an arena that is
        // not present in the array of arenas that are marked as conflicting
        for (u32 j = 0; j < conflict_count; ++j)
        {
            if (*arena_ptr == *conflict_ptr)
            {
                has_conflict = true;
                break;
            }

            conflict_ptr += 1;
        }

        if (!has_conflict)
            return *arena_ptr;

        arena_ptr += 1;
    }

    return nullptr;
}
