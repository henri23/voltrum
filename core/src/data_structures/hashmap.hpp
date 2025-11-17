#pragma once

#include "core/asserts.hpp"
#include "core/logger.hpp"
#include "core/string.hpp"
#include "defines.hpp"
#include "math/math.hpp"
#include "memory/memory.hpp"
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

enum class Hashmap_Item_State : u8 { EMPTY, OCCUPIED };

// Implement Robin Hood hashing
template <typename T> struct Hashmap_Item {
    T value;
    Hashmap_Item_State state;
    char key[50]; // Cached key for linear probing in the collision tail
    u32 distance; // Will store the probe sequence length for each item

    FORCE_INLINE b8 empty() const { return state == Hashmap_Item_State::EMPTY; }
};

constexpr u64 HASHMAP_DEFAULT_CAPACITY = 2;

// NOTE: This hashmap implementation does not maange the lifetime of the
// elements stored inside. If the elements T hold resources, these elements
// should be first cleaned manually before the hashmap goes out of scope

// Currently, the hashmap is not resizable
template <typename T> struct Hashmap {
    u64 capacity; // The largest power of two that accomodates all elements
    u64 count;
    Hashmap_Item<T>* memory;

    // TODO: Add custom allocator memory management
    // Allocator* allocator;

    FORCE_INLINE Hashmap(u64 requested_capacity = HASHMAP_DEFAULT_CAPACITY) {
        RUNTIME_ASSERT_MSG(requested_capacity >= 2,
            "The capacity of the hashmap must be greater than 2");

        // Use the power of 2 ceiling value for hashmap size
        capacity = math_next_power_of_2(requested_capacity);
        count = 0;
        memory = static_cast<Hashmap_Item<T>*>(
            memory_allocate(sizeof(Hashmap_Item<T>) * capacity,
                Memory_Tag::HASHMAP));
    }

    FORCE_INLINE ~Hashmap() {
        memory_deallocate(memory,
            sizeof(Hashmap_Item<T>) * capacity,
            Memory_Tag::HASHMAP);

        capacity = 0;
        count = 0;
        memory = nullptr;
    }

    FORCE_INLINE b8 add(const char* key, const T* value) {

        if (string_length(key) >= 50) {
            CORE_ERROR("Hashmap key '%s' exceeds 50 characters", key);
            return false;
        }

        if (count == capacity) {
            CORE_WARN("Hashmap is full and cannot accept any more elements");
            return false;
        }

        // By exploiting the capacity as a power of two instead of using the %
        // operator to map all possible hash addresses inside the bounds of the
        // array, we instead just create a bitmask and apply a bitwise and op.
        u64 address = hash_function(key) & (capacity - 1);

        RUNTIME_ASSERT(address < capacity);

        Hashmap_Item<T> current_item = {};
        current_item.value = *value;
        string_copy(current_item.key, key, 50);
        current_item.distance = 0;
        // Mark already as occupied because if we will use this element, we are
        // adding willingly the element inside the hash map
        current_item.state = Hashmap_Item_State::OCCUPIED;

        u64 probe = 0;

        // Collision detected
        while (probe < capacity) {
            // If the position is empty, write straight away
            if (memory[address].empty()) {
                memory[address] = current_item;
                count++;
                return true;
            }

            if (!memory[address].empty() &&
                string_check_equal(memory[address].key, current_item.key)) {
                CORE_WARN("Key '%s' is already present in the hashmap",
                    current_item.key);
                return false;
            }

            // If we are at distance x from the "proper" position and we
            // encouter an element that is very close to its own "proper"
            // position, instead of propagating the new value and exagerating
            // the tail of the current element's bucket, we swap the two
            // elements and instead start propagating the "rich" element
            if (memory[address].distance < current_item.distance) {
                swap(&memory[address], &current_item);
            }

            probe++;
            address = (address + 1) & (capacity - 1); // Wraparound the address
            current_item.distance++;
        }

        return false;
    }

    FORCE_INLINE b8 find(const char* key, T* out_value) {

        if (string_length(key) >= 50) {
            CORE_ERROR("Hashmap key '%s' exceeds 50 characters", key);
            return false;
        }

        u64 address = hash_function(key) & (capacity - 1);

        RUNTIME_ASSERT(address < capacity);

        u64 probe = 0;

        // Collision detected
        while (probe < capacity) {
            if (memory[address].empty()) {
                CORE_WARN("Key '%s' is not present inside the hashmap", key);
                return false;
            }

            if (string_check_equal(memory[address].key, key)) {
                *out_value = memory[address].value;
                return true;
            }

            probe++;
            address = next_address(address); // Wraparound the address
        }

        return false;
    }

    FORCE_INLINE b8 remove(const char* key) {

        if (string_length(key) >= 50) {
            CORE_ERROR("Hashmap key '%s' exceeds 50 characters", key);
            return false;
        }

        u64 address = hash_function(key) & (capacity - 1);

        RUNTIME_ASSERT(address < capacity);

        u64 probe = 0;
        b8 found = false;
        u64 found_address = -1; // Invalid address

        // Linear probing after computing hash function
        while (probe < capacity) {
            if (memory[address].empty()) {
                CORE_WARN("Key '%s' is not present inside the hashmap", key);
                return false;
            }

            if (string_check_equal(memory[address].key, key)) {
                found = true;
                found_address = address;
                break;
            }

            probe++;
            address = next_address(address); // Wraparound the address
        }

        // To delete the item we shift every element until we reach a new
        // element properly aligned by 1, in an attempt to improve their
        // respective distances
        if (found) {
            probe = 0;
            address = found_address;

            while (probe < capacity) {
                // If the next element in line is already at the optimal
                // location we stop
                auto* next_item = &memory[next_address(address)];
                if (next_item->distance == 0 || next_item->empty()) {
                    memory_zero(&memory[address], sizeof(Hashmap_Item<T>));
                    count--;
                    return true;
                }

                // In theory we can optimize this further because tecnically,
                // I just need to copy the data from the next items, without
                // propagating the data to be deleted to the next location,
                // however it is easier to visualize the algorithm this way
                memory[address] = *next_item;
                memory[address].distance--;

                probe++;
                address = next_address(address);
            }
        }

        return false;
    }

    // Helper function for looping over the hashmap in a for loop without
    // handling the empty/not-empty logic. Useful when destroying the resources
    // tied or managed by the hashmap elements T
    FORCE_INLINE u64 next_occupied_index(u64 start_index) const {
        for (u64 i = start_index; i < capacity; ++i) {
            if (!memory[i].empty()) {
                return i;
            }
        }
        return capacity;
    }

    FORCE_INLINE void debug_log_table() const {
        CORE_INFO("HashMap debug view with count='%llu' and capacity='%llu')",
            count,
            capacity);

        CORE_INFO(" Slot | Dist | Key");

        u64 printed = 0;
        for (u64 idx = next_occupied_index(0);
            idx < capacity && printed < count;
            idx = next_occupied_index(idx + 1)) {

            const auto* slot = &memory[idx];
            CORE_INFO("%5llu | %4u | %-44s", idx, slot->distance, slot->key);

            ++printed;
        }
    }

    FORCE_INLINE void swap(Hashmap_Item<T>* item1, Hashmap_Item<T>* item2) {
        T temp_value = item1->value;
        char temp_key[50];
        string_copy(temp_key, item1->key, 50);
        u32 temp_distance = item1->distance;
        Hashmap_Item_State temp_state = item1->state;

        item1->value = item2->value;
        string_copy(item1->key, item2->key, 50);
        item1->distance = item2->distance;
        item1->state = item2->state;

        item2->value = temp_value;
        string_copy(item2->key, temp_key, 50);
        item2->distance = temp_distance;
        item2->state = temp_state;
    }

    FORCE_INLINE u64 next_address(u64 current_address) {
        return (current_address + 1) & (capacity - 1);
    }

    FORCE_INLINE u64 hash_function(const char* key) {
        // For the hash function I am using the Fowler-Noll-Vo hash function
        // Reference:
        // https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
        // This combination of FNV offset basis and FNV prime gives the best
        // collision performance for 64 bit addressing
        u64 hash = 0xcbf29ce484222325; // FNV offset basis
        while (*key) { // until we run into the null propagator character
            hash ^= static_cast<u8>(*key++);
            hash *= 0x00000100000001b3; // FNV prime
        }
        return hash;
    }
};
