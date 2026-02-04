#ifdef DEBUG_BUILD

#include "arena_debug.hpp"
#include "arena.hpp"

#include "core/logger.hpp"
#include "platform/platform.hpp"

internal_var Arena               *debug_arena    = nullptr;
internal_var Arena_Debug_Registry *debug_registry = nullptr;

void
arena_debug_init()
{
    // Create the debug arena directly (bypasses registry since it is not
    // initialized yet, arena_debug_register will early-return)
    debug_arena    = arena_create(64 * MiB, 64 * KiB);
    debug_registry = push_struct(debug_arena, Arena_Debug_Registry);

    debug_registry->active_count = 0;
    for (u32 i = 0; i < ARENA_DEBUG_MAX_ARENAS; ++i)
    {
        debug_registry->entries[i].arena           = nullptr;
        debug_registry->entries[i].active          = false;
        debug_registry->entries[i].records         = nullptr;
        debug_registry->entries[i].record_count    = 0;
        debug_registry->entries[i].record_capacity = 0;
    }

    CORE_INFO("Arena debug registry initialized");
}

void
arena_debug_shutdown()
{
    if (debug_arena)
    {
        CORE_INFO("Arena debug registry shutdown");
        // Deregister the debug arena itself before releasing
        debug_registry = nullptr;
        arena_release(debug_arena);
        debug_arena = nullptr;
    }
}

void
arena_debug_register(Arena *arena)
{
    if (!debug_registry)
        return;

    // Don't track the debug arena itself
    if (arena == (Arena *)debug_arena->memory)
        return;

    // Find a free slot
    for (u32 i = 0; i < ARENA_DEBUG_MAX_ARENAS; ++i)
    {
        Arena_Debug_Entry *entry = &debug_registry->entries[i];

        if (!entry->active)
        {
            entry->arena    = arena;
            entry->active   = true;

            entry->records  =
                push_array(debug_arena,
                           Arena_Allocation_Record,
                           ARENA_DEBUG_INITIAL_RECORD_CAP);
            entry->record_count    = 0;
            entry->record_capacity = ARENA_DEBUG_INITIAL_RECORD_CAP;

            debug_registry->active_count++;
            return;
        }
    }

    CORE_WARN("Arena debug registry full (%u arenas tracked). "
              "Increase ARENA_DEBUG_MAX_ARENAS.",
              ARENA_DEBUG_MAX_ARENAS);
}

void
arena_debug_deregister(Arena *arena)
{
    if (!debug_registry)
        return;

    for (u32 i = 0; i < ARENA_DEBUG_MAX_ARENAS; ++i)
    {
        Arena_Debug_Entry *entry = &debug_registry->entries[i];

        if (entry->active && entry->arena == arena)
        {
            entry->arena        = nullptr;
            entry->active       = false;
            // Records memory stays in debug_arena (bump allocator, no free)
            entry->records      = nullptr;
            entry->record_count = 0;
            entry->record_capacity = 0;

            debug_registry->active_count--;
            return;
        }
    }
}

void
arena_debug_record_push(
    Arena      *arena,
    u64         offset,
    u64         size,
    u64         padding,
    const char *file,
    s32         line)
{
    if (!debug_registry)
        return;

    for (u32 i = 0; i < ARENA_DEBUG_MAX_ARENAS; ++i)
    {
        Arena_Debug_Entry *entry = &debug_registry->entries[i];

        if (entry->active && entry->arena == arena)
        {
            // Grow the records array if full
            if (entry->record_count >= entry->record_capacity)
            {
                u32 new_capacity = entry->record_capacity * 2;
                auto *new_records =
                    push_array(debug_arena,
                               Arena_Allocation_Record,
                               new_capacity);

                platform_copy_memory(
                    new_records,
                    entry->records,
                    entry->record_count *
                        sizeof(Arena_Allocation_Record));

                entry->records        = new_records;
                entry->record_capacity = new_capacity;
                // Old records are leaked in debug_arena (acceptable for
                // debug-only feature)
            }

            Arena_Allocation_Record *record =
                &entry->records[entry->record_count];

            record->offset  = offset;
            record->size    = size;
            record->padding = padding;
            record->file    = file;
            record->line    = line;

            entry->record_count++;
            return;
        }
    }
}

void
arena_debug_record_pop_to(Arena *arena, u64 new_position)
{
    if (!debug_registry)
        return;

    for (u32 i = 0; i < ARENA_DEBUG_MAX_ARENAS; ++i)
    {
        Arena_Debug_Entry *entry = &debug_registry->entries[i];

        if (entry->active && entry->arena == arena)
        {
            // Remove all records whose offset >= new_position
            while (entry->record_count > 0)
            {
                Arena_Allocation_Record *last =
                    &entry->records[entry->record_count - 1];

                if (last->offset >= new_position)
                {
                    entry->record_count--;
                }
                else
                {
                    break;
                }
            }
            return;
        }
    }
}

Arena_Debug_Registry *
arena_debug_get_registry()
{
    return debug_registry;
}

#endif // DEBUG_BUILD
