#include "vulkan_swapchain.hpp"

#include "core/logger.hpp"
#include "memory/arena.hpp"
#include "memory/memory.hpp"

#include "defines.hpp"

#include "renderer/vulkan/vulkan_device.hpp"
#include "renderer/vulkan/vulkan_types.hpp"

void create_swapchain(Vulkan_Context *context,
    u32 width,
    u32 height,
    Vulkan_Swapchain *out_swapchain) {

    // To create the swapchain we need first to make 3 decisions:
    // 1. Choose the color format that we want
    // 2. Choose the present mode that we want
    // 3. Specify the extent of the image (size). This will be immutable so if
    // 	  the screen gets resized, the swapchain must be recreated with the new
    // 	  size

    // Retrieve the device specific swapchain support info
    Vulkan_Swapchain_Support_Info *swapchain_info =
        &context->device.swapchain_info;

    // Choose prefered format from available formats of the logical device
    // The formats and present modes array are setup on device selection in
    // vulkan_device_query_swapchain_capabilities()

    b8 found = false;
    for (u32 i = 0; i < swapchain_info->formats_count; ++i) {
        if (swapchain_info->formats[i].format == VK_FORMAT_B8G8R8A8_UNORM &&
            swapchain_info->formats[i].colorSpace ==
                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            out_swapchain->image_format = swapchain_info->formats[i];
            found = true;
            CORE_INFO(
                "Selected swapchain format: B8G8R8A8_UNORM with SRGB_NONLINEAR "
                "color space");
            break;
        }
    }

    // If the requested format was not found then pick the first one available
    if (!found) {
        out_swapchain->image_format = swapchain_info->formats[0];
        CORE_WARN(
            "Preferred format not found, using fallback: format=%d, "
            "colorSpace=%d",
            out_swapchain->image_format.format,
            out_swapchain->image_format.colorSpace);
    }

    // From the time we get initially the swapchain support during device
    // selection, until the renderer comes here, the present modes may have
    // changed so we query a second time to get the most up-to-date properties
    vulkan_device_query_swapchain_capabilities(context->persistent_arena,
        context->device.physical_device,
        context->surface,
        &context->device.swapchain_info);

    // For present modes all GPU must implement VK_PRESENT_MODE_FIFO_KHR, and
    // it is the most similar to how OpenGl works, however my first choice
    // would be VK_PRESENT_MODE_MAILBOX_KHR
    VkPresentModeKHR selected_present_mode;
    found = false;

    for (u32 i = 0; i < swapchain_info->present_modes_count; ++i) {
        if (swapchain_info->present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {

            selected_present_mode = swapchain_info->present_modes[i];
            found = true;
            break;
        }
    }

    if (!found)
        selected_present_mode = VK_PRESENT_MODE_FIFO_KHR;

    // Log the selected presentation mode
    const char *present_mode_name = "UNKNOWN";
    switch (selected_present_mode) {
    case VK_PRESENT_MODE_IMMEDIATE_KHR:
        present_mode_name = "IMMEDIATE (no vsync)";
        break;
    case VK_PRESENT_MODE_MAILBOX_KHR:
        present_mode_name = "MAILBOX (triple buffering)";
        break;
    case VK_PRESENT_MODE_FIFO_KHR:
        present_mode_name = "FIFO (vsync)";
        break;
    case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
        present_mode_name = "FIFO_RELAXED (adaptive vsync)";
        break;
    default:
        break;
    }
    CORE_INFO("Vulkan presentation mode: %s", present_mode_name);

    // The swap extent is the resolution of the swap chain images and it is
    // almost always equal to the resolution of the windows (with the exception)
    // of Apple's Retina Displas (TODO).

    VkExtent2D actual_extent = {
        CLAMP(width,
            (u32)swapchain_info->capabilities.minImageExtent.width,
            (u32)swapchain_info->capabilities.maxImageExtent.width),
        CLAMP(height,
            (u32)swapchain_info->capabilities.minImageExtent.height,
            (u32)swapchain_info->capabilities.maxImageExtent.height),
    };

    VkSwapchainCreateInfoKHR create_info = {
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};

    create_info.surface = context->surface;

    // Set the minimum image count in the swapchain, but nothing forbids the
    // swapchain to have more images than this. Prefer 3 swapchain images
    u32 image_count =
        CLAMP(swapchain_info->capabilities.minImageCount + 1, 0, 3);

    out_swapchain->max_in_flight_frames = 2;

    create_info.minImageCount = image_count;

    create_info.imageFormat = out_swapchain->image_format.format;
    create_info.imageColorSpace = out_swapchain->image_format.colorSpace;
    create_info.imageExtent = actual_extent;

    // imageArrayLayers specifies the amount of layers each image consists of.
    // This is always 1 unless we are developing a stereoscopic 3D application
    create_info.imageArrayLayers = 1;

    // If we want to use images just as a canvas where we draw we use the color
    // attachment bit as ImageUsage. It is also possible to first draw into
    // anther image and do some post-processing operation and later render the
    // image with VK_IMAGE_USAGE_TRANSFER_DST_BIT. This requires memory ops.
    // to transfer the rendered image to the swap chain
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Since the same swapchain will be used accross multiple queues, we need
    // to specify how the images that are accessed by multiple queues will be
    // handled.
    // - VK_SHARING_MODE_EXCLUSIVE: an image is owned by one queue family at a
    // time and ownership must be explicitly transferred before using it in
    // another queue family
    // - VK_SHARING_MODE_CONCURRENT: images can be used across multiple queues
    // without explicit ownership transfer
    u32 queue_family_indices[] = {context->device.graphics_queue_index,
        context->device.present_queue_index};

    if (context->device.graphics_queue_index !=
        context->device.present_queue_index) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;     // optional
        create_info.pQueueFamilyIndices = nullptr; // optional
    }

    create_info.preTransform = swapchain_info->capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = selected_present_mode;
    create_info.clipped = VK_TRUE;

    // In theory Vulkan can destroy the an existing swapchain by passing
    // the reference here, however it is better to explicitly destroy
    // any older swapchain
    create_info.oldSwapchain = VK_NULL_HANDLE;

    VK_CHECK(vkCreateSwapchainKHR(context->device.logical_device,
        &create_info,
        context->allocator,
        &out_swapchain->handle))

    CORE_DEBUG("Vulkan swapchain instance created");

    out_swapchain->extent = actual_extent;

    context->current_frame = 0;
    out_swapchain->image_count = 0;

    vkGetSwapchainImagesKHR(context->device.logical_device,
        out_swapchain->handle,
        &out_swapchain->image_count,
        nullptr);

    if (!out_swapchain->images) {
        out_swapchain->images = push_array(context->persistent_arena,
            VkImage,
            out_swapchain->image_count);
    }

    if (!out_swapchain->views) {
        out_swapchain->views = push_array(context->persistent_arena,
            VkImageView,
            out_swapchain->image_count);
    }

    vkGetSwapchainImagesKHR(context->device.logical_device,
        out_swapchain->handle,
        &out_swapchain->image_count,
        out_swapchain->images);

    for (u32 i = 0; i < out_swapchain->image_count; ++i) {
        VkImageViewCreateInfo view_info = {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};

        view_info.image = out_swapchain->images[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = out_swapchain->image_format.format;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        // The components field allows us to swizzle the color channels. If
        // we map all the channels to the red channel, we will get a monochrome
        // texture. I am keeping the default mapping here
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // The subresourceRange field describes what the image's purpose is and
        // which part of the image should be accessed. The images in this engine
        // will be color targets without any mipmapping levels or multiple
        // layers
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(context->device.logical_device,
            &view_info,
            context->allocator,
            &out_swapchain->views[i]));
    }

    CORE_DEBUG("Created images and image views for swapchain");

    // Z-buffer creation
    // Create a depth image. The depth image is an image where the depth info
    // is written too. However the swapchain does not create this image for us
    // so it must be created manually
    // vulkan_image_create(context,
    //     VK_IMAGE_TYPE_2D,
    //     actual_extent.width,
    //     actual_extent.height,
    //     context->device.depth_format, // Set above
    //     VK_IMAGE_TILING_OPTIMAL,
    //     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    //     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    //     true,
    //     VK_IMAGE_ASPECT_DEPTH_BIT,
    //     &out_swapchain->depth_attachment);

    CORE_INFO("Vulkan swapchain successfully created.");
}

void vulkan_swapchain_create(Vulkan_Context *context,
    u32 width,
    u32 height,
    Vulkan_Swapchain *out_swapchain) {

    create_swapchain(context, width, height, out_swapchain);
}

void vulkan_swapchain_recreate(Vulkan_Context *context,
    u32 width,
    u32 height,
    Vulkan_Swapchain *out_swapchain) {

    CORE_DEBUG("Destroying previous swapchain...");

    vulkan_swapchain_destroy(context, out_swapchain);

    CORE_DEBUG("Recreating swapchain with sizes { %d ; %d }", width, height);

    create_swapchain(context, width, height, out_swapchain);
}

void vulkan_swapchain_destroy(Vulkan_Context *context,
    Vulkan_Swapchain *swapchain) {

    vkDeviceWaitIdle(context->device.logical_device);

    // Destroy the images that we create
    // vulkan_image_destroy(context, &swapchain->depth_attachment);

    CORE_DEBUG("Destroying image views... Found %d views",
        swapchain->image_count);

    // Only destroy the views, because the images of the swapchain are managed
    // by Vulkan itself so there is no need to destroy them
    for (u32 i = 0; i < swapchain->image_count; ++i) {
        vkDestroyImageView(context->device.logical_device,
            swapchain->views[i],
            context->allocator);
    }

    CORE_DEBUG("All image views destroyed");

    CORE_INFO("Destroying Vulkan swapchain...");

    swapchain->views  = nullptr;
    swapchain->images = nullptr;

    vkDestroySwapchainKHR(context->device.logical_device,
        swapchain->handle,
        context->allocator);

    CORE_INFO("Swapchain destroyed.");
}
