#pragma once

#include "core/asserts.hpp"
#include "core/logger.hpp"
#include "defines.hpp"
#include "math/math.hpp"
#include "memory/arena.hpp"
#include "memory/memory.hpp"
#include "utils/string.hpp"
// TODO: Use C++ 20 modules for exporting template data structures to avoid
// parsing the classes for every translation unit

// Simple hashmap implementation. For non-pointer values the map retains a copy
// of the value. For pointer types the hashtmap does not take ownership of the
// pointer, so the lifetime of the object should be managed outside the hashmap

// This hashmap is an open addressing flat hashmap implementation, since voltrum
// does not satifsy the main reasons to why we would want to use a closed
// addressing hashmap:
// 1. 	Unbounded or unpredictable growth - In voltrum we usually know
// 		beforehand the amount of data that will be stored in the hashmap
// and we can resize in compile time, without requiring resizing dynamically
// 2.	Highly variable key lifetime - The lifetime of the
// 		elements stored in the buckets will be predictable, usually tied
// to the lifetime of the application for the resources
// 3. 	High load factors - Since the number of the elements will be known in
//		compile time, we can just resize in compile time to keep the
//		load factor pretty low
// The hashmap will have power of two ceiling capacities, so that we can apply
// the modulus operation by simple bitmask operations

// Implement Robin Hood hashing
template <typename T>
struct Hashmap_Item
{
    T      value;
    b8     is_occupied;
    String key;      // Arena-owned copy of the key
    u32    distance; // Will store the probe sequence length for each item
};

constexpr u64 HASHMAP_DEFAULT_CAPACITY = 16;

// NOTE: This hashmap implementation does not maange the lifetime of the
// elements stored inside. If the elements T hold resources, these elements
// should be first cleaned manually before the hashmap goes out of scope

// Currently, the hashmap is not resizable
template <typename T>
struct Hashmap
{
    u64 capacity; // The largest power of two that accomodates all elements
    u64 count;
    Hashmap_Item<T> *items;

    Arena *_allocator; // Add arena pointer to make the hashmap be backed by an
                       // arena

    FORCE_INLINE
    Hashmap()
    {
        capacity = 0;
        count    = 0;
        items    = nullptr;
    }

    FORCE_INLINE void
    init(Arena *allocator, u64 requested_capacity = HASHMAP_DEFAULT_CAPACITY)
    {
        RUNTIME_ASSERT_MSG(requested_capacity >= 2,
            "hashmap_init - The capacity must be greater than 2");

        RUNTIME_ASSERT_MSG(items == nullptr,
            "hashmap_init - Hashmap already initialized. Call clear() before "
            "re-initializing");

        RUNTIME_ASSERT_MSG(allocator != nullptr,
            "hashmap_init - Invalid arena allocator");

        _allocator = allocator;

        // Use the power of 2 ceiling value for hashmap size
        capacity = math_next_power_of_2(requested_capacity);
        count    = 0;
        items    = push_array(_allocator, Hashmap_Item<T>, capacity);
    }

    FORCE_INLINE b8
    add(String key, const T *value, b8 overwrite = false)
    {
        if (items == nullptr)
        {
            CORE_ERROR(
                "Hashmap not initialized. Call init() before using add()");
            return false;
        }

        if (count == capacity)
        {
            CORE_WARN("Hashmap is full and cannot accept any more elements");
            return false;
        }

        // By exploiting the capacity as a power of two instead of using the %
        // operator to map all possible hash addresses inside the bounds of the
        // array, we instead just create a bitmask and apply a bitwise and op.
        u64 address = string_hash(key) & (capacity - 1);

        RUNTIME_ASSERT(address < capacity);

        Hashmap_Item<T> current_item = {};
        current_item.value           = *value;
        current_item.key             = string_copy(_allocator, key);
        current_item.distance        = 0;
        // Mark already as occupied because if we will use this element, we are
        // adding willingly the element inside the hash map
        current_item.is_occupied = true;

        u64 probe = 0;

        // Collision detected
        while (probe < capacity)
        {
            // If the position is empty, write straight away
            if (!items[address].is_occupied)
            {
                items[address] = current_item;
                count++;
                return true;
            }

            if (items[address].is_occupied &&
                string_match(items[address].key, current_item.key))
            {
                if (overwrite)
                {
                    items[address].value = current_item.value;
                    return true;
                }

                CORE_WARN("Key '%.*s' is already present in the hashmap",
                    (int)current_item.key.size,
                    current_item.key.buff);
                return false;
            }

            // If we are at distance x from the "proper" position and we
            // encouter an element that is very close to its own "proper"
            // position, instead of propagating the new value and exagerating
            // the tail of the current element's bucket, we swap the two
            // elements and instead start propagating the "rich" element
            if (items[address].distance < current_item.distance)
            {
                Hashmap_Item<T> temp = items[address];
                items[address]       = current_item;
                current_item         = temp;
            }

            probe++;
            address = (address + 1) & (capacity - 1); // Wraparound the address
            current_item.distance++;
        }

        return false;
    }

    FORCE_INLINE b8
    find_ptr(String key, T **out_ptr)
    {
        if (items == nullptr)
        {
            CORE_ERROR(
                "Hashmap not initialized. Call init() before using find()");
            return false;
        }

        u64 address = string_hash(key) & (capacity - 1);

        RUNTIME_ASSERT(address < capacity);

        u64 probe = 0;

        // Collision detected
        while (probe < capacity)
        {
            if (!items[address].is_occupied)
            {
                CORE_WARN("Key '%.*s' is not present inside the hashmap",
                    (int)key.size,
                    key.buff);
                return false;
            }

            if (string_match(items[address].key, key))
            {
                *out_ptr = &items[address].value;
                return true;
            }

            probe++;
            address = next_address(address); // Wraparound the address
        }

        return false;
    }

    FORCE_INLINE b8
    find(String key, T *out_copy)
    {
        if (items == nullptr)
        {
            CORE_ERROR(
                "Hashmap not initialized. Call init() before using find()");
            return false;
        }

        u64 address = string_hash(key) & (capacity - 1);

        RUNTIME_ASSERT(address < capacity);

        u64 probe = 0;

        // Collision detected
        while (probe < capacity)
        {
            if (!items[address].is_occupied)
            {
                CORE_WARN("Key '%.*s' is not present inside the hashmap",
                    (int)key.size,
                    key.buff);
                return false;
            }

            if (string_match(items[address].key, key))
            {
                memory_copy(out_copy, &items[address].value, sizeof(T));
                return true;
            }

            probe++;
            address = next_address(address); // Wraparound the address
        }

        return false;
    }

    FORCE_INLINE b8
    remove(String key)
    {
        if (items == nullptr)
        {
            CORE_ERROR(
                "Hashmap not initialized. Call init() before using remove()");
            return false;
        }

        u64 address = string_hash(key) & (capacity - 1);

        RUNTIME_ASSERT(address < capacity);

        u64 probe         = 0;
        b8  found         = false;
        u64 found_address = -1; // Invalid address

        // Linear probing after computing hash function
        while (probe < capacity)
        {
            if (!items[address].is_occupied)
            {
                CORE_WARN("Key '%.*s' is not present inside the hashmap",
                    (int)key.size,
                    key.buff);
                return false;
            }

            if (string_match(items[address].key, key))
            {
                found         = true;
                found_address = address;
                break;
            }

            probe++;
            address = next_address(address); // Wraparound the address
        }

        // To delete the item we shift every element until we reach a new
        // element properly aligned by 1, in an attempt to improve their
        // respective distances
        if (found)
        {
            probe   = 0;
            address = found_address;

            while (probe < capacity)
            {
                // If the next element in line is already at the optimal
                // location we stop
                auto *next_item = &items[next_address(address)];
                if (next_item->distance == 0 || !next_item->is_occupied)
                {
                    memory_zero(&items[address], sizeof(Hashmap_Item<T>));
                    count--;
                    return true;
                }

                // In theory we can optimize this further because tecnically,
                // I just need to copy the data from the next items, without
                // propagating the data to be deleted to the next location,
                // however it is easier to visualize the algorithm this way
                items[address] = *next_item;
                items[address].distance--;

                probe++;
                address = next_address(address);
            }
        }

        return false;
    }

    // Helper function for looping over the hashmap in a for loop without
    // handling the empty/not-empty logic. Useful when destroying the resources
    // tied or managed by the hashmap elements T
    FORCE_INLINE u64
    next_occupied_index(u64 start_index) const
    {
        for (u64 i = start_index; i < capacity; ++i)
        {
            if (items[i].is_occupied)
            {
                return i;
            }
        }
        return capacity;
    }

    FORCE_INLINE void
    debug_log_table() const
    {
        CORE_INFO("HashMap debug view with count='%llu' and capacity='%llu')",
            count,
            capacity);

        CORE_INFO(" Slot | Dist | Key");

        u64 printed = 0;
        for (u64 idx = next_occupied_index(0);
            idx < capacity && printed < count;
            idx = next_occupied_index(idx + 1))
        {

            const auto *slot = &items[idx];
            CORE_INFO("%5llu | %4u | %.*s",
                idx,
                slot->distance,
                (int)slot->key.size,
                slot->key.buff);

            ++printed;
        }
    }

    FORCE_INLINE u64
    next_address(u64 current_address)
    {
        return (current_address + 1) & (capacity - 1);
    }

    FORCE_INLINE b8
    full()
    {
        return count == capacity;
    }
};
