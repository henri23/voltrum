#include "vulkan_buffer.hpp"

#include "core/logger.hpp"
#include "memory/memory.hpp"

#include "vulkan_command_buffer.hpp"

b8 vulkan_buffer_create(Vulkan_Context *context,
    u64 size,
    VkBufferUsageFlags usage,
    u32 memory_property_flags,
    b8 bind_on_create,
    Vulkan_Buffer *out_buffer) {

    memory_zero(out_buffer, sizeof(Vulkan_Buffer));
    out_buffer->total_size = size;
    out_buffer->usage = usage;
    out_buffer->memory_property_flags = memory_property_flags;

    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode =
        VK_SHARING_MODE_EXCLUSIVE; // Only used in one queue

    VK_CHECK(vkCreateBuffer(context->device.logical_device,
        &buffer_info,
        context->allocator,
        &out_buffer->handle));

    // Gather memory requirements
    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(context->device.logical_device,
        out_buffer->handle,
        &requirements);

    out_buffer->memory_index =
        context->find_memory_index(requirements.memoryTypeBits,
            out_buffer->memory_property_flags);

    if (out_buffer->memory_index == -1) {
        CORE_ERROR(
            "Unable to create vulkan buffer because the required memory type "
            "index was not found.");
        return false;
    }

    VkMemoryAllocateInfo allocate_info = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocate_info.allocationSize = requirements.size;
    allocate_info.memoryTypeIndex = (u32)out_buffer->memory_index;

    VkResult result = vkAllocateMemory(context->device.logical_device,
        &allocate_info,
        context->allocator,
        &out_buffer->memory);

    if (result != VK_SUCCESS) {
        CORE_ERROR(
            "Unable to create vulkan buffer due to required memory allocation "
            "failure. Error: %i",
            result);
        return false;
    }

    if (bind_on_create) {
        vulkan_buffer_bind(context, out_buffer, 0);
    }

    return true;
}

void vulkan_buffer_destroy(Vulkan_Context *context, Vulkan_Buffer *buffer) {
    if (buffer->memory) {
        vkFreeMemory(context->device.logical_device,
            buffer->memory,
            context->allocator);
        buffer->memory = nullptr;
    }
    if (buffer->handle) {
        vkDestroyBuffer(context->device.logical_device,
            buffer->handle,
            context->allocator);
        buffer->handle = nullptr;
    }

    buffer->total_size = 0;
    buffer->usage = (VkBufferUsageFlagBits)0;
    buffer->is_locked = false;
}

b8 vulkan_buffer_resize(Vulkan_Context *context,
    u64 new_size,
    Vulkan_Buffer *buffer,
    VkQueue queue,
    VkCommandPool pool) {

    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = new_size;
    buffer_info.usage = buffer->usage;
    buffer_info.sharingMode =
        VK_SHARING_MODE_EXCLUSIVE; // Only used in one queue

    VkBuffer new_buffer;
    VK_CHECK(vkCreateBuffer(context->device.logical_device,
        &buffer_info,
        context->allocator,
        &new_buffer));

    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(context->device.logical_device,
        new_buffer,
        &requirements);

    VkMemoryAllocateInfo allocate_info = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocate_info.allocationSize = requirements.size;
    allocate_info.memoryTypeIndex = (u32)buffer->memory_index;

    VkDeviceMemory new_memory;
    VkResult result = vkAllocateMemory(context->device.logical_device,
        &allocate_info,
        context->allocator,
        &new_memory);

    if (result != VK_SUCCESS) {
        CORE_ERROR(
            "Unable to resize vulkan buffer due to required memory allocation "
            "failure. Error: %i",
            result);
        return false;
    }

    VK_CHECK(vkBindBufferMemory(context->device.logical_device,
        new_buffer,
        new_memory,
        0));

    vulkan_buffer_copy_to(context,
        pool,
        0,
        queue,
        buffer->handle,
        0,
        new_buffer,
        0,
        buffer->total_size);

    vkDeviceWaitIdle(context->device.logical_device);

    if (buffer->memory) {
        vkFreeMemory(context->device.logical_device,
            buffer->memory,
            context->allocator);
        buffer->memory = nullptr;
    }
    if (buffer->handle) {
        vkDestroyBuffer(context->device.logical_device,
            buffer->handle,
            context->allocator);
        buffer->handle = nullptr;
    }

    buffer->total_size = new_size;
    buffer->memory = new_memory;
    buffer->handle = new_buffer;

    return true;
}

void vulkan_buffer_bind(Vulkan_Context *context,
    Vulkan_Buffer *buffer,
    u64 offset) {
    VK_CHECK(vkBindBufferMemory(context->device.logical_device,
        buffer->handle,
        buffer->memory,
        offset));
}

// Basically we take whatever is stored in the buffer and map it to the return
// value data
void *vulkan_buffer_lock_memory(Vulkan_Context *context,
    Vulkan_Buffer *buffer,
    u64 offset,
    u64 size,
    u32 flags) {

    void *data;
    VK_CHECK(vkMapMemory(context->device.logical_device,
        buffer->memory,
        offset,
        size,
        flags,
        &data));

    return data;
}

void vulkan_buffer_unlock_memory(Vulkan_Context *context,
    Vulkan_Buffer *buffer) {
    vkUnmapMemory(context->device.logical_device, buffer->memory);
}

// Copy from the buffer's memory
void vulkan_buffer_load_data(Vulkan_Context *context,
    Vulkan_Buffer *buffer,
    u64 offset,
    u64 size,
    u32 flags,
    const void *data) {

    void *data_ptr;

    VK_CHECK(vkMapMemory(context->device.logical_device,
        buffer->memory,
        offset,
        size,
        flags,
        &data_ptr));

    memory_copy(data_ptr, data, size);
    vkUnmapMemory(context->device.logical_device, buffer->memory);
}

void vulkan_buffer_copy_to(Vulkan_Context *context,
    VkCommandPool pool,
    VkFence fence,
    VkQueue queue,
    VkBuffer source,
    u64 source_offset,
    VkBuffer dest,
    u64 dest_offset,
    u64 size) {

    vkQueueWaitIdle(queue);
    Vulkan_Command_Buffer temp_command_buffer;
    vulkan_command_buffer_startup_single_use(context,
        pool,
        &temp_command_buffer);

    VkBufferCopy copy_region;
    copy_region.srcOffset = source_offset;
    copy_region.dstOffset = dest_offset;
    copy_region.size = size;

    vkCmdCopyBuffer(temp_command_buffer.handle, source, dest, 1, &copy_region);

    vulkan_command_buffer_end_single_use(context,
        pool,
        &temp_command_buffer,
        queue);
}
