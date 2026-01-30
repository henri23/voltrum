#pragma once

#include "defines.hpp"

#include "core/asserts.hpp"
#include "core/logger.hpp"
#include "memory/arena.hpp"

constexpr u64 DEFAULT_RING_QUEUE_CAPACITY = 256;

// Fixed-size ring buffer queue backed by an arena.
// When full, new pushes are dropped and a warning is logged.
template <typename T>
struct Ring_Queue
{
    u64 capacity;
    u64 count;
    u64 head; // Next read index
    u64 tail; // Next write index

    T     *elements;
    Arena *_allocator; // Should not be accessed externally

    FORCE_INLINE void
    init(Arena *allocator, u64 fixed_capacity = DEFAULT_RING_QUEUE_CAPACITY)
    {
        RUNTIME_ASSERT(allocator != nullptr);
        RUNTIME_ASSERT(fixed_capacity > 0);

        _allocator = allocator;
        capacity   = fixed_capacity;
        count      = 0;
        head       = 0;
        tail       = 0;
        elements   = push_array(_allocator, T, capacity);
    }

    FORCE_INLINE b8
    is_full()
    {
        return count >= capacity;
    }

    FORCE_INLINE b8
    is_empty()
    {
        return count == 0;
    }

    FORCE_INLINE b8
    enqueue(const T &value)
    {
        if (is_full())
        {
            CORE_WARN("Ring queue full - element dropped (capacity: %llu)",
                      capacity);
            return false;
        }

        elements[tail] = value;
        tail           = (tail + 1) % capacity;
        count += 1;

        return true;
    }

    FORCE_INLINE b8
    dequeue(T *out)
    {
        RUNTIME_ASSERT(out != nullptr);

        if (is_empty())
            return false;

        *out = elements[head];
        head = (head + 1) % capacity;
        count -= 1;

        return true;
    }

    FORCE_INLINE T *
    peek()
    {
        if (is_empty())
            return nullptr;

        return &elements[head];
    }

    FORCE_INLINE void
    reset()
    {
        count = 0;
        head  = 0;
        tail  = 0;
    }
};
