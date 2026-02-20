#include "vulkan_device.hpp"

#include "core/logger.hpp"
#include "core/thread_context.hpp"
#include "data_structures/dynamic_array.hpp"
#include "memory/memory.hpp"
#include "renderer/vulkan/vulkan_types.hpp"
#include "utils/string.hpp"
#include <vulkan/vulkan_core.h>

struct Device_Queue_Indices {
    u32 graphics_family_index;
    u32 transfer_family_index;
    u32 present_family_index;
    u32 compute_family_index;
};

INTERNAL_FUNC b8 is_device_suitable(Arena *scratch,
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    const VkPhysicalDeviceProperties *properties,
    const VkPhysicalDeviceFeatures *features,
    const Vulkan_Physical_Device_Requirements *requirements,
    Vulkan_Swapchain_Support_Info *out_swapchain_info,
    Device_Queue_Indices *out_indices);

INTERNAL_FUNC b8 select_physical_device(Vulkan_Context *context,
    Vulkan_Physical_Device_Requirements *requirements);

INTERNAL_FUNC b8 create_logical_device(Vulkan_Context *context);

b8 vulkan_device_initialize(Vulkan_Context *context,
    Vulkan_Physical_Device_Requirements *requirements) {

    // Select physical device in the machine
    if (!select_physical_device(context, requirements)) {
        CORE_FATAL("Failed to select physical device. Aborting...");
        return false;
    }

    // Create logical device
    if (!create_logical_device(context)) {
        CORE_FATAL("Failed to create logical device. Aborting...");
        return false;
    }

    return true;
}

INTERNAL_FUNC const char *vulkan_depth_format_string(VkFormat format) {
    switch (format) {
    case VK_FORMAT_D32_SFLOAT:
        return "VK_FORMAT_D32_SFLOAT";
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return "VK_FORMAT_D32_SFLOAT_S8_UINT";
    case VK_FORMAT_D24_UNORM_S8_UINT:
        return "VK_FORMAT_D24_UNORM_S8_UINT";
    default:
        return "VK_FORMAT_UNDEFINED";
    }
}

// TODO: Get back to this method as it is not clear enough
b8 vulkan_device_detect_depth_format(Vulkan_Device *device) {

    const u64 candidate_count = 3;

    // Specify the types of z-buffer that we are happy to use
    VkFormat candidates[3]{
        VK_FORMAT_D32_SFLOAT, // 32-bit signaed float, 32 bits depth component
        VK_FORMAT_D32_SFLOAT_S8_UINT, // Two components, 32 bit depth, 8 bit
                                      // stencil
        VK_FORMAT_D24_UNORM_S8_UINT}; // Two components, 8 bit stencil, 24 depth

    // In this case the depth and stencil buffer are merged into one buffer
    u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for (u32 i = 0; i < candidate_count; ++i) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(device->physical_device,
            candidates[i],
            &properties);

        if ((properties.linearTilingFeatures & flags) == flags) {
            device->depth_format = candidates[i];
            CORE_INFO("Selected depth format: %s",
                vulkan_depth_format_string(device->depth_format));
            return true;
        }

        if ((properties.optimalTilingFeatures & flags) == flags) {
            device->depth_format = candidates[i];
            CORE_INFO("Selected depth format: %s",
                vulkan_depth_format_string(device->depth_format));
            return true;
        }
    }

    return false;
}

b8 select_physical_device(Vulkan_Context *context,
    Vulkan_Physical_Device_Requirements *requirements) {
    u32 physical_device_count = 0;

    // Retrieve the list of available GPUs with vulkan support
    vkEnumeratePhysicalDevices(context->instance,
        &physical_device_count,
        nullptr);

    // Check if there's at least one GPU that supports Vulkan
    if (physical_device_count == 0)
        return false;

    Scratch_Arena scratch = scratch_begin(nullptr, 0);

    VkPhysicalDevice *physical_devices_array =
        push_array(scratch.arena, VkPhysicalDevice, physical_device_count);

    vkEnumeratePhysicalDevices(context->instance,
        &physical_device_count,
        physical_devices_array);

    // Evaluate GPUs -> If mutliple GPUs are present in the machine,
    // we need to pick the most "qualified" one
    for (u32 i = 0; i < physical_device_count; ++i) {

        VkPhysicalDeviceProperties device_properties;
        VkPhysicalDeviceFeatures device_features;
        VkPhysicalDeviceMemoryProperties device_memory_properties;

        vkGetPhysicalDeviceProperties(physical_devices_array[i],
            &device_properties);
        vkGetPhysicalDeviceFeatures(physical_devices_array[i],
            &device_features);
        vkGetPhysicalDeviceMemoryProperties(physical_devices_array[i],
            &device_memory_properties);

        Device_Queue_Indices queue_indices;

        // Score the GPUs based on the properties they provide
        b8 result = is_device_suitable(scratch.arena,
            physical_devices_array[i],
            context->surface,
            &device_properties,
            &device_features,
            requirements,
            &context->device.swapchain_info,
            &queue_indices);

        if (result) {
            CORE_INFO("Selected device: '%s'", device_properties.deviceName);
            switch (device_properties.deviceType) {
            default:
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                CORE_INFO("GPU type is unknown.");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                CORE_INFO("GPU type is discrete.");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                CORE_INFO("GPU type is integrated.");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                CORE_INFO("GPU type is CPU.");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                CORE_INFO("GPU type is virtual.");
                break;
            }

            CORE_DEBUG("GPU Driver Version: %d.%d.%d",
                VK_VERSION_MAJOR(device_properties.driverVersion),
                VK_VERSION_MINOR(device_properties.driverVersion),
                VK_VERSION_PATCH(device_properties.driverVersion));

            CORE_DEBUG("Vulkan API Version: %d.%d.%d",
                VK_VERSION_MAJOR(device_properties.apiVersion),
                VK_VERSION_MINOR(device_properties.apiVersion),
                VK_VERSION_PATCH(device_properties.apiVersion));

            for (u32 j = 0; j < device_memory_properties.memoryHeapCount; ++j) {
                f32 memory_size =
                    device_memory_properties.memoryHeaps[j].size / (float)GiB;
                if (device_memory_properties.memoryHeaps[j].flags &
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                    CORE_DEBUG("Local GPU memory: %.2f GiB", memory_size);
                } else {
                    CORE_DEBUG("Shared GPU memory: %.2f GiB", memory_size);
                }
            }

            b8 support_device_local_host_visible_feature = false;
            for (u32 i = 0; i < device_memory_properties.memoryTypeCount; ++i) {
                if (((device_memory_properties.memoryTypes[i].propertyFlags &
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0) &&
                    ((device_memory_properties.memoryTypes[i].propertyFlags &
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0)) {
                    support_device_local_host_visible_feature = true;
                    CORE_DEBUG(
                        "Selected GPU support the device local host visible "
                        "flag");
                    break;
                }
            }

            // Store device handle in the vulkan context
            context->device.physical_device = physical_devices_array[i];
            context->device.physical_device_properties = device_properties;
            context->device.physical_device_features = device_features;
            context->device.physical_device_memory = device_memory_properties;
            context->device.supports_device_local_host_visible =
                support_device_local_host_visible_feature;

            // Store indices for queue instantiation later
            context->device.graphics_queue_index =
                queue_indices.graphics_family_index;
            context->device.transfer_queue_index =
                queue_indices.transfer_family_index;
            context->device.compute_queue_index =
                queue_indices.compute_family_index;
            context->device.present_queue_index =
                queue_indices.present_family_index;

            break;
        }
    }

    scratch_end(scratch);

    if (context->device.physical_device) {
        return true;
    }

    return false;
}

b8 create_logical_device(Vulkan_Context *context) {
    CORE_INFO("Creating logical device...");
    // At least one for the graphics queue because otherwise
    // the GPU would not be eligible
    u32 distinct_queue_family_indices_count = 1;

    b8 does_transfer_share_queue = context->device.transfer_queue_index ==
                                   context->device.graphics_queue_index;

    b8 does_present_share_queue = context->device.present_queue_index ==
                                  context->device.graphics_queue_index;

    if (!does_transfer_share_queue)
        distinct_queue_family_indices_count++;

    if (!does_present_share_queue)
        distinct_queue_family_indices_count++;

    Scratch_Arena scratch = scratch_begin(nullptr, 0);

    u32 *queue_family_indices =
        push_array(scratch.arena, u32, distinct_queue_family_indices_count);

    queue_family_indices[0] = context->device.graphics_queue_index;

    if (!does_transfer_share_queue)
        queue_family_indices[1] = context->device.transfer_queue_index;

    if (!does_present_share_queue)
        queue_family_indices[2] = context->device.present_queue_index;

    // Information for the queues that we want to request
    VkDeviceQueueCreateInfo *queue_create_infos = push_array(scratch.arena,
        VkDeviceQueueCreateInfo,
        distinct_queue_family_indices_count);

    u32 max_queue_count = 2;

    // TODO: Since I already do this operation during the selection of the GPU
    // maybe I could save the properties of the queue family of the selected
    // GPU to be used here
    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(context->device.physical_device,
        &queue_family_count,
        nullptr);

    VkQueueFamilyProperties *queue_family_props = push_array(scratch.arena,
        VkQueueFamilyProperties,
        queue_family_count);

    vkGetPhysicalDeviceQueueFamilyProperties(context->device.physical_device,
        &queue_family_count,
        queue_family_props);

    f32 queue_priorities[2] = {1.0f, 1.0f};
    for (u32 i = 0; i < distinct_queue_family_indices_count; ++i) {
        queue_create_infos[i].sType =
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[i].queueFamilyIndex = queue_family_indices[i];

        // If we are on the graphics queue, instantiate 2 queues, however we
        // need to first check if there are 2 queues available
        // queue_create_infos[i].queueCount = (i == 0) ? 2 : 1;

        u32 available_queue_count =
            queue_family_props[queue_family_indices[i]].queueCount;

        if (i == 0 && available_queue_count >= 2)
            queue_create_infos[i].queueCount = 2;
        else
            queue_create_infos[i].queueCount = 1;

        queue_create_infos[i].pQueuePriorities = queue_priorities;
        queue_create_infos[i].flags = 0;
        queue_create_infos[i].pNext = nullptr;
    }

    VkPhysicalDeviceFeatures device_features_to_request = {};
    device_features_to_request.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo logical_device_create_info = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};

    logical_device_create_info.pQueueCreateInfos = queue_create_infos;
    logical_device_create_info.queueCreateInfoCount =
        distinct_queue_family_indices_count;
    logical_device_create_info.pEnabledFeatures = &device_features_to_request;

    // Request swapchain extension for physical device
    const char *required_extensions[4] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    u32         required_extensions_count = 1;

#ifdef PLATFORM_APPLE
    required_extensions[required_extensions_count++] =
        "VK_KHR_portability_subset";
#endif

    logical_device_create_info.ppEnabledExtensionNames = required_extensions;
    logical_device_create_info.enabledExtensionCount =
        required_extensions_count;

    // Depracated, for clarity explicitly set them to uninitialized
    logical_device_create_info.enabledLayerCount = 0;
    logical_device_create_info.ppEnabledLayerNames = nullptr;

    VK_CHECK(vkCreateDevice(context->device.physical_device,
        &logical_device_create_info,
        context->allocator,
        &context->device.logical_device));

    CORE_INFO("Logical device created.");

    // Get handles for all requested queues
    vkGetDeviceQueue(context->device.logical_device,
        context->device.graphics_queue_index,
        0,
        &context->device.graphics_queue);

    vkGetDeviceQueue(context->device.logical_device,
        context->device.transfer_queue_index,
        0,
        &context->device.transfer_queue);

    vkGetDeviceQueue(context->device.logical_device,
        context->device.present_queue_index,
        0,
        &context->device.presentation_queue);

    CORE_INFO("Queues obtained");

    VkCommandPoolCreateInfo pool_create_info = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_create_info.queueFamilyIndex = context->device.graphics_queue_index;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VK_CHECK(vkCreateCommandPool(context->device.logical_device,
        &pool_create_info,
        context->allocator,
        &context->device.graphics_command_pool));

    CORE_INFO("Graphics command pool created");

    scratch_end(scratch);

    return true;
}

// TODO: 	For now the algorithm just checks if the current GPU fullfills
// 			the requirements, and if so it breaks, so if there are
// 			mulitple GPUs that can fulfill those requirements, the
// 			first one gets selected, not necessarily the best

b8 is_device_suitable(Arena *scratch,
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    const VkPhysicalDeviceProperties *properties,
    const VkPhysicalDeviceFeatures *features,
    const Vulkan_Physical_Device_Requirements *requirements,
    Vulkan_Swapchain_Support_Info *out_swapchain_info,
    Device_Queue_Indices *out_indices) {

    // Initialize the family index to a unreasonable value so that it is
    // evident whether or not a queue family that supports given commands
    // is found
    out_indices->graphics_family_index = -1;
    out_indices->compute_family_index = -1;
    out_indices->present_family_index = -1;
    out_indices->transfer_family_index = -1;

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device,
        &queue_family_count,
        nullptr);

    auto queue_family_properties_array = push_array(scratch,
        VkQueueFamilyProperties,
        queue_family_count);

    vkGetPhysicalDeviceQueueFamilyProperties(device,
        &queue_family_count,
        queue_family_properties_array);

    if (requirements->discrete_gpu &&
        properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        CORE_DEBUG("Device is not a discrete GPU. Skipping.");
        return false;
    }

    CORE_INFO("Graphics | Present | Compute | Transfer | Name");

    // If a queue family offers Transfer commands capability on top of other
    // types of commands, maybe it is not the best possible options, so the
    // target family queue would be a queue "dedicated" to transfer commands.
    // This means that the more additional commands to the transfer ones a
    // queue will have, the less optimal it is to be chosen as for transfer.
    // Obviously if it is the only family queue that provides transfer we will
    // still pick it
    u8 min_transfer_score = 255;

    for (u32 i = 0; i < queue_family_count; ++i) {
        u8 current_transfer_score = 0;

        if (queue_family_properties_array[i].queueFlags &
            VK_QUEUE_GRAPHICS_BIT) {
            out_indices->graphics_family_index = i;
            ++current_transfer_score;
        }

        if (queue_family_properties_array[i].queueFlags &
            VK_QUEUE_COMPUTE_BIT) {
            out_indices->compute_family_index = i;
            ++current_transfer_score;
        }

        if (queue_family_properties_array[i].queueFlags &
            VK_QUEUE_SPARSE_BINDING_BIT)
            ++current_transfer_score;

        if (queue_family_properties_array[i].queueFlags &
            VK_QUEUE_VIDEO_DECODE_BIT_KHR)
            ++current_transfer_score;

        if (queue_family_properties_array[i].queueFlags &
            VK_QUEUE_VIDEO_ENCODE_BIT_KHR)
            ++current_transfer_score;

        // Mark this family as the go to transfer queue family only if it is
        // lower than the current minimum
        if (queue_family_properties_array[i].queueFlags &
                VK_QUEUE_TRANSFER_BIT &&
            current_transfer_score <= min_transfer_score) {
            out_indices->transfer_family_index = i;
            min_transfer_score = current_transfer_score;
        }

        VkBool32 present_support = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device,
            i,
            surface,
            &present_support);

        if (present_support) {
            out_indices->present_family_index = i;
        }
    }

    CORE_INFO("       %d |       %d |       %d |        %d | %s",
        out_indices->graphics_family_index,
        out_indices->present_family_index,
        out_indices->compute_family_index,
        out_indices->transfer_family_index,
        properties->deviceName);

    if ((!requirements->graphics ||
            (requirements->graphics &&
                out_indices->graphics_family_index != -1)) &&
        (!requirements->compute ||
            (requirements->compute &&
                out_indices->compute_family_index != -1)) &&
        (!requirements->transfer ||
            (requirements->transfer &&
                out_indices->transfer_family_index != -1)) &&
        (!requirements->present ||
            (requirements->present &&
                out_indices->present_family_index != -1))) {

        vulkan_device_query_swapchain_capabilities(device,
            surface,
            out_swapchain_info);

        if (out_swapchain_info->formats_count == -1 ||
            out_swapchain_info->present_modes_count == -1) {
            CORE_DEBUG("Swapchain is not fully supported. Skipping device.");
            return false;
        }

        CORE_INFO("Device '%s' has swapchain support", properties->deviceName);

        CORE_INFO("Device meets all the requirements.");

        CORE_TRACE("Graphics queue family index: %d",
            out_indices->graphics_family_index);
        CORE_TRACE("Compute queue family index: %d",
            out_indices->compute_family_index);
        CORE_TRACE("Transfer queue family index: %d",
            out_indices->transfer_family_index);
        CORE_TRACE("Present queue family index: %d",
            out_indices->present_family_index);

        // Check whether the device supports all the required device level
        // extensions (namelly the swapchain extension)
        if (requirements->device_extension_names->size > 0) {
            u32 available_extensions_count = 0;

            VK_CHECK(vkEnumerateDeviceExtensionProperties(device,
                nullptr,
                &available_extensions_count,
                nullptr));

            auto extension_properties = push_array(scratch,
                VkExtensionProperties,
                available_extensions_count);

            VK_CHECK(vkEnumerateDeviceExtensionProperties(device,
                nullptr,
                &available_extensions_count,
                extension_properties));

            for (u32 i = 0;
                i < requirements->device_extension_names->size;
                ++i) {
                b8 found = false;

                for (u32 j = 0; j < available_extensions_count; ++j) {
                    if (string_match(
                            STR(extension_properties[j].extensionName),
                            STR((*requirements->device_extension_names)[i]))) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    CORE_INFO(
                        "Required extension not found: '%s', skipping device "
                        "'%s'",
                        (*requirements->device_extension_names)[i],
                        properties->deviceName);

                    return false;
                }
            }
        }

        return true;
    }

    return false;
}

void vulkan_device_query_swapchain_capabilities(
    VkPhysicalDevice              device,
    VkSurfaceKHR                  surface,
    Vulkan_Swapchain_Support_Info *out_swapchain_info) {

    out_swapchain_info->formats_count = 0;
    out_swapchain_info->present_modes_count = 0;

    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device,
        surface,
        &out_swapchain_info->capabilities));

    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device,
        surface,
        &out_swapchain_info->formats_count,
        nullptr));

    if (out_swapchain_info->formats_count != 0) {
        RUNTIME_ASSERT_MSG(
            out_swapchain_info->formats_count <= VULKAN_MAX_SURFACE_FORMATS,
            "Surface format count exceeds VULKAN_MAX_SURFACE_FORMATS");

        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device,
            surface,
            &out_swapchain_info->formats_count,
            out_swapchain_info->formats));
    }

    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device,
        surface,
        &out_swapchain_info->present_modes_count,
        nullptr));

    if (out_swapchain_info->present_modes_count != 0) {
        RUNTIME_ASSERT_MSG(
            out_swapchain_info->present_modes_count <= VULKAN_MAX_PRESENT_MODES,
            "Present mode count exceeds VULKAN_MAX_PRESENT_MODES");

        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device,
            surface,
            &out_swapchain_info->present_modes_count,
            out_swapchain_info->present_modes));
    }
}

void vulkan_device_shutdown(Vulkan_Context *context) {

    CORE_DEBUG("Destroying command pools...");

    vkDestroyCommandPool(context->device.logical_device,
        context->device.graphics_command_pool,
        context->allocator);

    if (context->device.logical_device) {
        CORE_INFO("Destroying logical device recource...");

        vkDestroyDevice(context->device.logical_device, context->allocator);
        context->device.logical_device = nullptr;
    }

    context->device.swapchain_info.formats_count = 0;
    context->device.swapchain_info.present_modes_count = 0;

    context->device.presentation_queue = nullptr;
    context->device.graphics_queue = nullptr;
    context->device.transfer_queue = nullptr;

    // Since the physical device is not created, but just obtained, there is
    // nothing to free, exept the utilized resources
    CORE_INFO("Releasing physical device resource...");
    context->device.physical_device = nullptr;

    context->device.graphics_queue_index = -1;
    context->device.transfer_queue_index = -1;
    context->device.compute_queue_index = -1;
}
