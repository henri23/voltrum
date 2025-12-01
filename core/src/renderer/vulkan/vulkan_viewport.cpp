#include "vulkan_viewport.hpp"
#include "memory/memory.hpp"

void vulkan_viewport_create(Vulkan_Context* context,
    u32 width,
    u32 height,
    Vulkan_Viewport* out_viewport) {

    u32 swapchain_image_count = context->swapchain.image_count;

    if (!out_viewport->images) {
        out_viewport->images = static_cast<VkImage*>(
            memory_allocate(sizeof(VkImage) * swapchain_image_count,
                Memory_Tag::RENDERER));
    }

    if (!out_viewport->views) {
        out_viewport->views = static_cast<VkImageView*>(
            memory_allocate(sizeof(VkImageView) * swapchain_image_count,
                Memory_Tag::RENDERER));
    }
}

void vulkan_viewport_destroy(Vulkan_Context* context,
    Vulkan_Viewport* viewport) {}
