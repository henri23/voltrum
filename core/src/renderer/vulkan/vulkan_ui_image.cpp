#include "vulkan_ui_image.hpp"
#include "core/logger.hpp"
#include "imgui_impl_vulkan.h"
#include "memory/memory.hpp"
#include "vulkan_command_buffer.hpp"
#include "vulkan_image.hpp"

void vulkan_ui_image_create(Vulkan_Context* context,
    u32 width,
    u32 height,
    VkFormat format,
    const void* pixel_data,
    u32 pixel_data_size,
    Vulkan_UI_Image* out_ui_image) {

    // Create the base Vulkan image
    vulkan_image_create(context,
        VK_IMAGE_TYPE_2D,
        width,
        height,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        true,
        VK_IMAGE_ASPECT_COLOR_BIT,
        &out_ui_image->base_image);

    // Initialize UI-specific fields
    out_ui_image->sampler = VK_NULL_HANDLE;
    out_ui_image->descriptor_set = VK_NULL_HANDLE;

    // Upload pixel data if provided
    if (pixel_data && pixel_data_size > 0) {
        // Create staging buffer
        VkBuffer staging_buffer;
        VkDeviceMemory staging_buffer_memory;

        VkBufferCreateInfo buffer_info = {};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = pixel_data_size;
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VK_CHECK(vkCreateBuffer(context->device.logical_device,
            &buffer_info,
            context->allocator,
            &staging_buffer));

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(context->device.logical_device,
            staging_buffer,
            &mem_requirements);

        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_requirements.size;
        s32 memory_type_index =
            context->find_memory_index(mem_requirements.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (memory_type_index == -1) {
            CORE_ERROR(
                "Failed to find suitable memory type for staging buffer");
            vkDestroyBuffer(context->device.logical_device,
                staging_buffer,
                context->allocator);
            return;
        }
        alloc_info.memoryTypeIndex = memory_type_index;

        VK_CHECK(vkAllocateMemory(context->device.logical_device,
            &alloc_info,
            context->allocator,
            &staging_buffer_memory));

        VK_CHECK(vkBindBufferMemory(context->device.logical_device,
            staging_buffer,
            staging_buffer_memory,
            0));

        // Copy pixel data to staging buffer
        void* data;
        vkMapMemory(context->device.logical_device,
            staging_buffer_memory,
            0,
            pixel_data_size,
            0,
            &data);
        memory_copy(data, pixel_data, pixel_data_size);
        vkUnmapMemory(context->device.logical_device, staging_buffer_memory);

        // Copy buffer to image
        Vulkan_Command_Buffer command_buffer;
        vulkan_command_buffer_startup_single_use(context,
            context->device.graphics_command_pool,
            &command_buffer);

        // Transition image layout for transfer destination
        vulkan_image_transition_layout(context,
            &command_buffer,
            &out_ui_image->base_image,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {out_ui_image->base_image.width,
            out_ui_image->base_image.height,
            1};

        vkCmdCopyBufferToImage(command_buffer.handle,
            staging_buffer,
            out_ui_image->base_image.handle,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region);

        // Transition image layout for shader reading
        vulkan_image_transition_layout(context,
            &command_buffer,
            &out_ui_image->base_image,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vulkan_command_buffer_end_single_use(context,
            context->device.graphics_command_pool,
            &command_buffer,
            context->device.graphics_queue);

        // Clean up staging buffer
        vkDestroyBuffer(context->device.logical_device,
            staging_buffer,
            context->allocator);
        vkFreeMemory(context->device.logical_device,
            staging_buffer_memory,
            context->allocator);
    }

    // Create descriptor set using ImGui's function with shared sampler
    out_ui_image->descriptor_set =
        ImGui_ImplVulkan_AddTexture(context->ui_linear_sampler,
            out_ui_image->base_image.view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    CORE_DEBUG("UI image created: %ux%u", width, height);
}

void vulkan_ui_image_destroy(Vulkan_Context* context,
    Vulkan_UI_Image* ui_image) {

    // Clean up ImGui descriptor set
    if (ui_image->descriptor_set != VK_NULL_HANDLE) {
        ImGui_ImplVulkan_RemoveTexture(ui_image->descriptor_set);
        ui_image->descriptor_set = VK_NULL_HANDLE;
    }

    // Note: We don't own the sampler (it's the shared imgui_linear_sampler)
    ui_image->sampler = VK_NULL_HANDLE;

    // Destroy the base Vulkan image
    vulkan_image_destroy(context, &ui_image->base_image);

    CORE_DEBUG("UI image destroyed");
}
