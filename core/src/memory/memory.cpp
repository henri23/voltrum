#include "memory.hpp"

#include <stdio.h>
#include <string.h>

#include "core/logger.hpp"
#include "defines.hpp"
#include "platform/platform.hpp"
#include "utils/string.hpp"

struct Memory_Stats {
    u64 total_allocated;
    u64 tagged_allocations[(u64)Memory_Tag::MAX_ENTRIES];
};

// The memory system collects and stores metrics regarding memory utilization,
// however the memory management functions like allocate, dealloc etc. are
// standalone functions that should work even without a proper memory subsystem
// initialization. This means that the functions should work even without a
// state. This is useful especially in unit tests, where we would like to avoid
// memory subsystem initialization just to test our methods.
struct Memory_System_State {
    Memory_Stats stats;
    u64 allocations_count;
};

internal_var Memory_System_State state = {};

internal_var const char* memory_tag_strings[(u64)Memory_Tag::MAX_ENTRIES] = {
    "UNKNOWN  	:",
    "ARRAY   	:",
    "DARRAY   	:",
    "HASHMAP		:",
    "LINEAR_ALLOC	:",
    "EVENTS   	:",
    "STRING   	:",
    "CLIENT     	:",
    "INPUT 		:",
    "RENDERER 	:",
    "TEXTURE 	:",
    "MATERIAL 	:",
    "GEOMETRY 	:",
    "LOADERS		:",
    "APPLICATION	:",
    "UI		:",
    "LAYERS		:"};

void memory_init() {}

void memory_shutdown(void* state) {}

void* memory_allocate(u64 size, Memory_Tag tag) {
    if (tag == Memory_Tag::UNKNOWN) {
        CORE_WARN(
            "The memory is being initialized as UNKNOWN. Please allocated it "
            "with the proper tag");
    }

    state.stats.tagged_allocations[(u64)tag] += size;
    state.stats.total_allocated += size;
    state.allocations_count++;

    // Every chunk of memory will be set to 0 automatically
    void* block = platform_allocate(size, true);

    platform_zero_memory(block, size);

    return block;
}

void memory_deallocate(void* block, u64 size, Memory_Tag tag) {

    state.stats.tagged_allocations[(u64)tag] -= size;
    state.stats.total_allocated -= size;

    return platform_free(block, true);
}

void* memory_zero(void* block, u64 size) {
    return platform_zero_memory(block, size);
}

void* memory_copy(void* destination, const void* source, u64 size) {
    b8 is_overlap = false;

    u64 dest_addr = reinterpret_cast<u64>(destination);
    u64 source_addr = reinterpret_cast<u64>(source);

    if (source_addr == dest_addr) {
        CORE_WARN(
            "Method memory_copy() called with identical source and destination "
            "addresses. No action will occur");
        return destination;
    }

    // Since we are adding the size to the integer value of the address, the
    // compiler must copy 'size' bytes to the destination. When copying 'size'
    // bytes, the address value will be incremented by ('size' - 1). So if the
    // max_addr coincides with min_addr + size - 1 there will be overlap.
    if (source_addr > dest_addr && source_addr < dest_addr + size)
        is_overlap = true;
    else if (dest_addr > source_addr && dest_addr < source_addr + size)
        is_overlap = true;

    if (!is_overlap)
        return platform_copy_memory(destination, source, size);
    else {
        CORE_DEBUG(
            "Method memory_copy() called with overlapping regions of memory, "
            "using memmove() instead");
        return platform_move_memory(destination, source, size);
    }
}

void* memory_move(void* destination, const void* source, u64 size) {
    return platform_move_memory(destination, source, size);
}

void* memory_set(void* block, s32 value, u64 size) {
    return platform_set_memory(block, value, size);
}

void memory_get_current_usage(char* out_buf) {
    char utilization_buffer[5000] = "Summary of allocated memory (tagged):\n";

    u64 offset = strlen(
        utilization_buffer); // The offset is represented in number of bytes

    u64 max_tags = static_cast<u64>(Memory_Tag::MAX_ENTRIES);
    for (u32 i = 0; i < max_tags; ++i) {
        char usage_unit[4] = "XiB";
        f32 amount = 1.0f;

        if (state.stats.tagged_allocations[i] >= GIB) {
            usage_unit[0] = 'G';
            amount = (float)state.stats.tagged_allocations[i] / GIB;
        } else if (state.stats.tagged_allocations[i] >= MIB) {
            usage_unit[0] = 'M';
            amount = (float)state.stats.tagged_allocations[i] / MIB;
        } else if (state.stats.tagged_allocations[i] >= KIB) {
            usage_unit[0] = 'K';
            amount = (float)state.stats.tagged_allocations[i] / KIB;
        } else {
            usage_unit[0] = 'B';
            usage_unit[1] = 0; // Append a null termination character to
                               // overwrite the end of the string
            amount = (float)state.stats.tagged_allocations[i];
        }

        // snprintf returns the number of writen characters (aka bytes). It
        // returns negative number if there was an error
        s32 length = snprintf(utilization_buffer + offset,
            sizeof(utilization_buffer) - offset,
            "%s %.2f %s\n",
            memory_tag_strings[i],
            amount,
            usage_unit);

        offset += length;
    }

    // In order to use this buffer to another place we need to copy its value
    // into a dynamically allocated memory This is because the buffer will go
    // out of scope after we return and the value will be jibrish We need one
    // more byte for the null terminator character as the strlen disregards it
    u64 length = strlen(utilization_buffer);

    string_copy(out_buf, utilization_buffer);
}

u64 memory_get_allocations_count() { return state.allocations_count; }
