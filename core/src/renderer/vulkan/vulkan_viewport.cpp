#include "vulkan_viewport.hpp"
#include "core/logger.hpp"
#include "renderer/vulkan/vulkan_device.hpp"
#include "vulkan_image.hpp"

void vulkan_viewport_create(Vulkan_Context* context,
    u32 width,
    u32 height,
    Vulkan_Viewport* out_viewport) {

    // Read the format information already setup for the swapchain to remain
    // consistent with the image formats between the two
    // TODO: Make turn this into a function
    u32 swapchain_image_count = context->swapchain.image_count;

    Vulkan_Swapchain_Support_Info* swapchain_info =
        &context->device.swapchain_info;

    b8 found = false;
    for (u32 i = 0; i < swapchain_info->formats_count; ++i) {
        if (swapchain_info->formats[i].format == VK_FORMAT_B8G8R8A8_UNORM &&
            swapchain_info->formats[i].colorSpace ==
                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            out_viewport->image_format = swapchain_info->formats[i];
            found = true;

            CORE_INFO(
                "Selected viewport format: B8G8R8A8_UNORM with SRGB_NONLINEAR "
                "color space, same as swapchain");
            break;
        }
    }

    // If the requested format was not found then pick the first one available
    if (!found) {
        out_viewport->image_format = swapchain_info->formats[0];
        CORE_WARN(
            "Preferred format not found, using fallback: format=%d, "
            "colorSpace=%d",
            out_viewport->image_format.format,
            out_viewport->image_format.colorSpace);
    }

    out_viewport->framebuffer_width = width;
    out_viewport->framebuffer_height = height;

    // Set the extent for the viewport
    out_viewport->extent.width = width;
    out_viewport->extent.height = height;

    out_viewport->image_format.format = out_viewport->image_format.format;
    out_viewport->image_format.colorSpace =
        out_viewport->image_format.colorSpace;

    // Create color attachments for each swapchain image
    for (u32 i = 0; i < swapchain_image_count; ++i) {
        vulkan_image_create(
            context,
            VK_IMAGE_TYPE_2D,
            width,
            height,
            out_viewport->image_format.format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            true,
            VK_IMAGE_ASPECT_COLOR_BIT,
            &out_viewport->color_attachments[i]);
    }

    // Create depth buffer. The depth buffer is an image cotaining the depth
    // from the camera point of view
    if (!vulkan_device_detect_depth_format(&context->device)) {
        context->device.depth_format = VK_FORMAT_UNDEFINED;
        // TODO: Make this error recoverable by chosing a different format
        CORE_FATAL("Failed to find a supported depth format!");
    }

    // Create depth attachment
    vulkan_image_create(
        context,
        VK_IMAGE_TYPE_2D,
        width,
        height,
        context->device.depth_format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        true,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        &out_viewport->depth_attachment);

    // Note: Layout transitions are handled automatically by the renderpass
    // The viewport renderpass transitions from UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL

    CORE_INFO("Vulkan viewport successfully created.");
}

void vulkan_viewport_destroy(Vulkan_Context* context,
    Vulkan_Viewport* viewport) {

    u32 swapchain_image_count = context->swapchain.image_count;

    // Destroy color attachments
    for (u32 i = 0; i < swapchain_image_count; ++i) {
        vulkan_image_destroy(context, &viewport->color_attachments[i]);
    }

    // Destroy depth attachment
    vulkan_image_destroy(context, &viewport->depth_attachment);

    CORE_INFO("Vulkan viewport destroyed.");
}
