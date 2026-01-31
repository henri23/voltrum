#pragma once

#include "defines.hpp"

#include <cstddef>

#include "core/asserts.hpp"
#include "memory/arena.hpp"
#include "memory/memory.hpp"

template <typename T>
struct Memory_Pool
{
    struct Slot
    {
        Slot *next_free;
        b8    active;
        T     item;
    };

    using Pool_Callback = void (*)(T *item);

    Arena *_allocator;
    Slot  *slots;
    Slot  *first_free;
    u32    capacity;
    u32    active_count;

    FORCE_INLINE void
    init(Arena *allocator, u32 max_capacity)
    {
        RUNTIME_ASSERT_MSG(allocator != nullptr,
                           "Memory_Pool::init - allocator cannot be null");
        RUNTIME_ASSERT_MSG(max_capacity > 0,
                           "Memory_Pool::init - capacity must be > 0");

        _allocator   = allocator;
        capacity     = max_capacity;
        active_count = 0;
        slots        = push_array(_allocator, Slot, capacity);

        first_free = &slots[0];
        for (u32 i = 0; i < capacity - 1; ++i)
        {
            slots[i].next_free = &slots[i + 1];
            slots[i].active    = false;
        }
        slots[capacity - 1].next_free = nullptr;
        slots[capacity - 1].active    = false;
    }

    FORCE_INLINE T *
    acquire()
    {
        RUNTIME_ASSERT_MSG(first_free,
                           "Memory_Pool::acquire - pool exhausted");

        Slot *slot      = first_free;
        first_free      = slot->next_free;
        slot->next_free = nullptr;
        slot->active    = true;
        active_count++;

        memory_zero(&slot->item, sizeof(T));

        return &slot->item;
    }

    FORCE_INLINE void
    release(T *item)
    {
        // Recover the containing Slot from the item pointer by computing
        // the byte offset of the 'item' field within a Slot
        using Slot_Type = Slot;
        constexpr u64 item_offset = offsetof(Slot_Type, item);

        Slot *slot = reinterpret_cast<Slot *>(
            reinterpret_cast<u8 *>(item) - item_offset);

        RUNTIME_ASSERT_MSG(slot->active,
                           "Memory_Pool::release - item is not active");

        slot->active    = false;
        slot->next_free = first_free;
        first_free      = slot;
        active_count--;
    }

    FORCE_INLINE void
    for_each_active(Pool_Callback callback)
    {
        for (u32 i = 0; i < capacity; ++i)
        {
            if (slots[i].active)
            {
                callback(&slots[i].item);
            }
        }
    }
};
