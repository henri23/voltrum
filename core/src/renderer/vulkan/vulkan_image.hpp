#pragma once

#include "vulkan_types.hpp"

// Auxilliary functions to aid in image creation, since we create images for
// vulkan textures, depth image etc.
void vulkan_image_create(Vulkan_Context* context,
    VkImageType image_type,
    u32 width,
    u32 height,
    VkFormat format, // the format of the image
    VkImageTiling tiling,
    VkImageUsageFlags usage, // how will the image be used
    VkMemoryPropertyFlags memory_flags,
    b32 create_view,
    VkImageAspectFlags view_aspect_flags,
    Vulkan_Image* out_image);

void vulkan_image_view_create(Vulkan_Context* context,
    VkFormat format,
    Vulkan_Image* image,
    VkImageAspectFlags aspect_flags);

void vulkan_image_destroy(Vulkan_Context* context, Vulkan_Image* image);

// Takes the different image layouts into a new layout. It allows to optimize
// the image layout from the layout that it uses in disk to a layout that is
// optimized for the renderer
void vulkan_image_transition_layout(Vulkan_Context* context,
    Vulkan_Command_Buffer* command_buffer,
    Vulkan_Image* image,
    VkFormat format,
    VkImageLayout old_layout,
    VkImageLayout new_layout);

// Takes the data from the buffer and copies it over to the actual image
void vulkan_image_copy_from_buffer(Vulkan_Context* context,
    Vulkan_Image* image,
    VkBuffer buffer,
    Vulkan_Command_Buffer* command_buffer);
