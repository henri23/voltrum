#pragma once

#include "defines.hpp"
#include "vulkan_types.hpp"

b8 vulkan_buffer_create(Vulkan_Context *context,
    u64 size,
    VkBufferUsageFlags usage,
    u32 memory_property_flags,
    b8 bind_on_create,
    Vulkan_Buffer *out_buffer);

void vulkan_buffer_destroy(Vulkan_Context *context, Vulkan_Buffer *buffer);

b8 vulkan_buffer_resize(Vulkan_Context *context,
    u64 new_size,
    Vulkan_Buffer *buffer,
    VkQueue queue,
    VkCommandPool pool);

void vulkan_buffer_bind(Vulkan_Context *context,
    Vulkan_Buffer *buffer,
    u64 offset);

void *vulkan_buffer_lock_memory(Vulkan_Context *context,
    Vulkan_Buffer *buffer,
    u64 offset,
    u64 size,
    u32 flags);

void vulkan_buffer_unlock_memory(Vulkan_Context *context,
    Vulkan_Buffer *buffer);

void vulkan_buffer_load_data(Vulkan_Context *context,
    Vulkan_Buffer *buffer,
    u64 offset,
    u64 size,
    u32 flags,
    const void *data);

void vulkan_buffer_copy_to(Vulkan_Context *context,
    VkCommandPool pool,
    VkFence fence,
    VkQueue queue,
    VkBuffer source,
    u64 source_offset,
    VkBuffer dest,
    u64 dest_offset,
    u64 size);
