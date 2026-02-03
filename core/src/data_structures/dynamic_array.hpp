#pragma once

#include "defines.hpp"

#include "core/asserts.hpp"
#include "memory/arena.hpp"
#include "memory/memory.hpp"

constexpr u64 DEFAULT_DYNAMIC_ARRAY_CAPACITY = 16;

// The autoarray is a dynamic array implementation that manages memory elements
// in a contigous memory layout
template <typename T>
struct Dynamic_Array
{
    struct Dynamic_Chunk
    {
        Dynamic_Chunk *next;

        u64 offset;
        T  *elements;
    };

    u64 granularity; // Size of each chunk equal to the initial capacity value
    u64 capacity;
    u64 size;

    Dynamic_Chunk *current;
    Dynamic_Chunk *first;

    Arena *_allocator; // Should not be accessed externally

    FORCE_INLINE void
    init(Arena *allocator, u64 initial_size = DEFAULT_DYNAMIC_ARRAY_CAPACITY)
    {
        RUNTIME_ASSERT(allocator != nullptr);
        RUNTIME_ASSERT(initial_size > 0);

        _allocator  = allocator;
        capacity    = initial_size;
        granularity = capacity;
        size        = 0;

        first = push_struct(_allocator, Dynamic_Chunk);

        current           = first;
        current->elements = push_array(_allocator, T, capacity);
        current->next     = nullptr;
        current->offset   = 0;

        // current->prev     = nullptr;
    }

    FORCE_INLINE T &
    operator[](u64 index)
    {
        RUNTIME_ASSERT_MSG(index < capacity && index >= 0,
                           "dynamic_array - Index out of bounds");

        u64 index_in_chunk = index % granularity;
        u64 chunk_number   = index / granularity;

        Dynamic_Chunk *chunk = first;

        for (u64 i = 0; i < capacity / granularity; ++i)
        {
            if (chunk_number == i)
                break;

            chunk = chunk->next;
        }

        RUNTIME_ASSERT_MSG(chunk != nullptr,
                           "dynamic_array - Error while indexing element");

        return *(chunk->elements + index_in_chunk);
    };

    FORCE_INLINE void
    _resize()
    {
        current->next = push_struct(_allocator, Dynamic_Chunk);
        current       = current->next;

        current->elements = push_array(_allocator, T, granularity);
        current->next     = nullptr;
        current->offset   = 0;

        capacity += granularity;
    }

    FORCE_INLINE void
    add(const T &value)
    {
        if (current->offset >= granularity && size >= capacity)
            _resize();

        RUNTIME_ASSERT_MSG(
            current->offset < granularity && size <= capacity,
            "dynamic_array_add - Not enough space to add new element");

        T *current_location = current->elements + current->offset;
        memory_copy(current_location, &value, sizeof(T));

        current->offset += 1;
        size += 1;
    }

    FORCE_INLINE void
    insert_at(u64 index, const T &value)
    {
        RUNTIME_ASSERT_MSG(index <= size,
                           "dynamic_array_insert_at - Index out of bounds");

        if (current->offset >= granularity && size >= capacity)
            _resize();

        // Shift elements from the end down to the insertion index
        for (u64 i = size; i > index; --i)
        {
            (*this)[i] = (*this)[i - 1];
        }

        (*this)[index] = value;

        // Update the current chunk offset and total size
        current->offset += 1;
        size += 1;
    }

    struct Iterator
    {
        Dynamic_Chunk *chunk;

        u64 index; // index within current chunk
        u64 granularity;

        FORCE_INLINE T &
        operator*()
        {
            return chunk->elements[index];
        }

        FORCE_INLINE Iterator &
        operator++()
        {
            ++index;
            if (index >= granularity && chunk->next != nullptr)
            {
                chunk = chunk->next;
                index = 0;
            }
            return *this;
        }

        FORCE_INLINE bool
        operator!=(const Iterator &other) const
        {
            return chunk != other.chunk || index != other.index;
        }
    };

    FORCE_INLINE Iterator
    begin()
    {
        return Iterator{first, 0, granularity};
    }

    FORCE_INLINE Iterator
    end()
    {
        return Iterator{current, current->offset, granularity};
    }
};
