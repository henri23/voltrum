#include "core/logger.hpp"

#include "memory/memory.hpp"
#include "platform/platform.hpp"
#include "systems/material_system.hpp"

#include "math/math_types.hpp"
#include "utils/string.hpp"

#include "vulkan_backend.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_command_buffer.hpp"
#include "vulkan_device.hpp"
#include "vulkan_image.hpp"
#include "vulkan_platform.hpp"
#include "vulkan_renderpass.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_types.hpp"
#include "vulkan_utils.hpp"
#include "vulkan_viewport.hpp"

#include "data_structures/auto_array.hpp"

#include "ui_backend/vulkan_ui_backend.hpp"

#include "shaders/vulkan_imgui_shader_pipeline.hpp"
#include "shaders/vulkan_material_shader_pipeline.hpp"

internal_var Vulkan_Context context;
internal_var u32 cached_swapchain_fb_width = 0;
internal_var u32 cached_swapchain_fb_height = 0;
internal_var u32 cached_viewport_fb_width = 0;
internal_var u32 cached_viewport_fb_height = 0;

// Forward declare messenger callback
VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data);

// Debug mode helper private functions
INTERNAL_FUNC b8 vulkan_create_debug_logger(VkInstance* instance);
INTERNAL_FUNC b8 vulkan_enable_validation_layers(
    Auto_Array<const char*>* required_layers_array);

// Needed for z-buffer image format sel/ection in vulkan_device
INTERNAL_FUNC s32 find_memory_index(u32 type_filter, u32 property_flags);
INTERNAL_FUNC b8 create_buffers(Vulkan_Context* context);

// Graphics presentation operations
INTERNAL_FUNC void create_command_buffers(Vulkan_Context* context);
INTERNAL_FUNC void regenerate_framebuffers();
INTERNAL_FUNC b8 present_frame(Renderer_Backend* backend);
INTERNAL_FUNC b8 get_next_image_index(Renderer_Backend* backend);

// The recreate_swapchain function is called both when a window resize event
// has ocurred and was published by the platform layer, or when a graphics ops.
// (i.e. present or get_next_image_index) finished with a non-optimal result
// code, which require the swapchain recreation. The flag is_resized_event
// descriminates between these two cases and makes sure not to overwrite
// renderpass size or read cached values, which are != 0 only when resize events
// occur.
INTERNAL_FUNC b8 recreate_swapchain(Renderer_Backend* backend,
    b8 is_resized_event);

// TODO: Temporary. Will move later
INTERNAL_FUNC void upload_data_range(Vulkan_Context* context,
    VkCommandPool pool,
    VkFence fence,
    VkQueue queue,
    Vulkan_Buffer* buffer,
    u64 offset,
    u64 size,
    const void* data) {

    VkBufferUsageFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    Vulkan_Buffer staging;

    vulkan_buffer_create(context,
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        flags,
        true,
        &staging);

    vulkan_buffer_load_data(context, &staging, 0, size, 0, data);

    vulkan_buffer_copy_to(context,
        pool,
        fence,
        queue,
        staging.handle,
        0,
        buffer->handle,
        offset,
        size);

    vulkan_buffer_destroy(context, &staging);
}

INTERNAL_FUNC void
free_data_range(Vulkan_Buffer* buffer, u64 offset, u64 size) {
    // NOTE: Empty because for now it is just a placeholder method
}

b8 vulkan_initialize(Renderer_Backend* backend, const char* app_name) {
    // Function pointer assignment
    context.find_memory_index = find_memory_index;

    // TODO: Custom allocator with memory arenas (Ryan Fleury tutorial)
    context.allocator = nullptr;

    // TODO: I do not like that the renderer calls the application layer,
    // since the dependency should be inverse.
    // application_get_framebuffer_size(&cached_framebuffer_width,
    //     &cached_framebuffer_height);

    platform_get_drawable_size(&cached_swapchain_fb_width,
        &cached_swapchain_fb_height);

    context.swapchain.framebuffer_width =
        (cached_swapchain_fb_width != 0) ? cached_swapchain_fb_width : 1280;

    context.swapchain.framebuffer_height =
        (cached_swapchain_fb_height != 0) ? cached_swapchain_fb_height : 720;

    cached_swapchain_fb_width = 0;
    cached_swapchain_fb_height = 0;

    context.viewport.framebuffer_width =
        (cached_viewport_fb_width != 0) ? cached_viewport_fb_width : 900;

    context.viewport.framebuffer_height =
        (cached_viewport_fb_height != 0) ? cached_viewport_fb_height : 550;

    cached_viewport_fb_width = 0;
    cached_viewport_fb_height = 0;

    VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};

    app_info.pNext = nullptr;
    app_info.pApplicationName = app_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Koala engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    // The API version should be set to the absolute minimum version of Vulkan
    // that the game engine requires to run, not to the version of the header
    // that is being used for development. This allows a wide assortment of
    // devices and platforms to run the koala engine
    app_info.apiVersion = VK_API_VERSION_1_2;

    // CreateInfo struct tells the Vulkan driver which global extensions
    // and validation layers to use.
    VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};

    createInfo.pApplicationInfo = &app_info;

#ifdef PLATFORM_APPLE
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    Auto_Array<const char*> required_extensions_array;

    // Get platform specific extensions (includes VK_KHR_surface and others)
    platform_get_required_extensions(&required_extensions_array);

    Auto_Array<const char*> required_layers_array;

// Only enable validation layer in debug builds
#ifdef DEBUG_BUILD
    // Add debug extensions
    required_extensions_array.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    CORE_DEBUG("Required VULKAN extensions:");
    for (u32 i = 0; i < required_extensions_array.length; ++i) {
        CORE_DEBUG(required_extensions_array[i]);
    }

    // Add validation layers
    vulkan_enable_validation_layers(&required_layers_array);
#endif

    // In Vulkan, applications need to explicitly specify the extensions that
    // they are going to use, and so the driver disables the extensions that
    // will not be used, so that the application cannot accidently start using
    // an extension in runtime
    createInfo.enabledExtensionCount = required_extensions_array.length;
    createInfo.ppEnabledExtensionNames = required_extensions_array.data;

    createInfo.enabledLayerCount = required_layers_array.length;
    createInfo.ppEnabledLayerNames = required_layers_array.data;

    VK_CHECK(
        vkCreateInstance(&createInfo, context.allocator, &context.instance));

#ifdef DEBUG_BUILD
    // Depends on the instance
    vulkan_create_debug_logger(&context.instance);
#endif

    Auto_Array<const char*> device_level_extension_requirements;

    // The swapchain is a device specific property (whether it supports it
    // or it doesn't) so we need to query specificly for the swapchain support
    // the device that we chose to use
    device_level_extension_requirements.push_back(
        VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    // Setup Vulkan device
    Vulkan_Physical_Device_Requirements device_requirements;
    device_requirements.compute = true;
    device_requirements.sampler_anisotropy = true;
    device_requirements.graphics = true;
    device_requirements.transfer = true;
    device_requirements.present = true;

#ifndef PLATFORM_APPLE
    device_requirements.discrete_gpu = true;
#else
    device_requirements.discrete_gpu = false;
#endif

    device_requirements.device_extension_names =
        &device_level_extension_requirements;

    // Create platform specific surface. Since the surface creation will
    // depend on the platform API, it is best that it is implemented in
    // the platform layer
    if (!platform_create_vulkan_surface(&context)) {
        CORE_FATAL("Failed to create platform specific surface");

        return false;
    }

    // Select physical device and create logical device
    if (!vulkan_device_initialize(&context, &device_requirements)) {
        CORE_FATAL(
            "No device that fulfills all the requirements was found in the "
            "machine");
        return false;
    }

    vulkan_swapchain_create(&context,
        context.swapchain.framebuffer_width,
        context.swapchain.framebuffer_height,
        &context.swapchain);

    vec4 swapchain_render_area = {0.0f,
        0.0f,
        (f32)context.swapchain.framebuffer_width,
        (f32)context.swapchain.framebuffer_height};

    vulkan_viewport_create(&context,
        context.viewport.framebuffer_width,
        context.viewport.framebuffer_height,
        &context.viewport);

    vec4 viewport_render_area = {0.0f,
        0.0f,
        (f32)context.viewport.framebuffer_width,
        (f32)context.viewport.framebuffer_height};

    vec4 clear_color = {0.0f, 0.0f, 0.2f, 1.0f};

    // Offscreen viewport renderpass
    vulkan_renderpass_create(&context,
        &context.viewport_renderpass,
        viewport_render_area,
        clear_color,
        1.0f,
        0,
        Renderpass_Clear_Flags::STENCIL_BUFFER |
            Renderpass_Clear_Flags::COLOR_BUFFER |
            Renderpass_Clear_Flags::DEPTH_BUFFER,
        false,
        true);

    // Application UI renderpass
    vulkan_renderpass_create(&context,
        &context.ui_renderpass,
        swapchain_render_area,
        clear_color,
        1.0f,
        0,
        Renderpass_Clear_Flags::COLOR_BUFFER,
        true,
        false);

    regenerate_framebuffers();

    create_command_buffers(&context);

    context.image_available_semaphores.resize(
        context.swapchain.max_in_flight_frames);

    context.render_finished_semaphores.resize(context.swapchain.image_count);

    VkSemaphoreCreateInfo semaphore_create_info = {
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    for (u8 i = 0; i < context.swapchain.max_in_flight_frames; ++i) {
        vkCreateSemaphore(context.device.logical_device,
            &semaphore_create_info,
            context.allocator,
            &context.image_available_semaphores[i]);

        // Create the fence in a signaled state, indicating that the first
        // frame has been "rendered". This will prevent the application from
        // waiting indefinetelly, because during boot-up there isn't any frame
        // to render. However we set this state to true, to trigger the next
        // frame rendering.
        VkFenceCreateInfo fence_create_info = {
            VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        vkCreateFence(context.device.logical_device,
            &fence_create_info,
            context.allocator,
            &context.in_flight_fences[i]);
    }

    // At this point in time, the images_in_flight fences are not yet created,
    // so we clear the array first. Basically the value should be nullptr when
    // not used
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {
        vkCreateSemaphore(context.device.logical_device,
            &semaphore_create_info,
            context.allocator,
            &context.render_finished_semaphores[i]);

        context.images_in_flight[i] = nullptr;
    }

    // Create builtin shaders
    if (!vulkan_material_shader_pipeline_create(&context,
            &context.material_shader)) {

        CORE_ERROR("Error loading built-in object shader");
        return false;
    }

    if (!vulkan_imgui_shader_pipeline_create(&context, &context.imgui_shader)) {

        CORE_ERROR("Error loading built-in imgui shader");
        return false;
    }

    // Initialize ImGui UI backend
    SDL_Window* window = platform_get_window();
    if (!vulkan_ui_backend_initialize(&context, window)) {
        CORE_ERROR("Failed to initialize ImGui UI backend");
        return false;
    }

    create_buffers(&context);
    CORE_INFO("Vulkan buffers created.");

    for (u32 i = 0; i < VULKAN_MAX_GEOMETRY_COUNT; ++i) {
        context.registered_geometries[i].id = INVALID_ID;
    }

    CORE_INFO("Vulkan backend initialized");

    return true;
}

void vulkan_shutdown(Renderer_Backend* backend) {
    // NOTE:	We might get problems when trying to shutdown the renderer while
    //			there are graphic operation still going on. First, it is
    // better 			to wait until all operations have completed, so
    // we do not get 			errors.
    vkDeviceWaitIdle(context.device.logical_device);

    // Wait for any pending main renderer operations to complete
    vkQueueWaitIdle(context.device.graphics_queue);

    // Reset command pools IMMEDIATELY to invalidate all command buffers
    // This must be done before any resource destruction to avoid
    // "resource still in use by command buffer" errors
    vkResetCommandPool(context.device.logical_device,
        context.device.graphics_command_pool,
        0);

    // Wait for device to finish all operations before cleanup
    CORE_DEBUG("Waiting for device to finish operations before UI cleanup...");
    vkDeviceWaitIdle(context.device.logical_device);

    // Shutdown ImGui UI backend
    vulkan_ui_backend_shutdown(&context);

    // Destroy all offscreen render target resources

    // Free dynamically allocated arrays

    CORE_DEBUG("Offscreen render targets destroyed");

    vulkan_buffer_destroy(&context, &context.object_vertex_buffer);
    vulkan_buffer_destroy(&context, &context.object_index_buffer);

    // Destroy shader modules
    vulkan_imgui_shader_pipeline_destroy(&context, &context.imgui_shader);
    vulkan_material_shader_pipeline_destroy(&context, &context.material_shader);

    // Destroy sync objects
    for (u8 i = 0; i < context.swapchain.max_in_flight_frames; ++i) {
        vkDestroySemaphore(context.device.logical_device,
            context.image_available_semaphores[i],
            context.allocator);

        vkDestroyFence(context.device.logical_device,
            context.in_flight_fences[i],
            context.allocator);
    }

    // Destroy render finished semaphores
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {
        vkDestroySemaphore(context.device.logical_device,
            context.render_finished_semaphores[i],
            context.allocator);
    }

    // Clear main renderer command buffer handles (already invalidated by pool
    // reset)
    for (u32 i = 0; i < context.swapchain.max_in_flight_frames; ++i) {
        context.command_buffers[i].handle = nullptr;
    }

    // First destroy the Vulkan objects
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {
        vkDestroyFramebuffer(context.device.logical_device,
            context.viewport.framebuffers[i],
            context.allocator);

        vkDestroyFramebuffer(context.device.logical_device,
            context.swapchain.framebuffers[i],
            context.allocator);
    }

    vulkan_renderpass_destroy(&context, &context.viewport_renderpass);
    vulkan_renderpass_destroy(&context, &context.ui_renderpass);

    vulkan_viewport_destroy(&context, &context.viewport);
    vulkan_swapchain_destroy(&context, &context.swapchain);

    vulkan_device_shutdown(&context);

    vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);

#ifdef DEBUG_BUILD
    CORE_DEBUG("Destroying Vulkan debugger...");
    if (context.debug_messenger) {

        VK_INSTANCE_LEVEL_FUNCTION(context.instance,
            vkDestroyDebugUtilsMessengerEXT);

        vkDestroyDebugUtilsMessengerEXT(context.instance,
            context.debug_messenger,
            context.allocator);
    }
#endif

    vkDestroyInstance(context.instance, context.allocator);

    CORE_DEBUG("Vulkan renderer shut down");
}

void vulkan_on_resized(Renderer_Backend* backend, u16 width, u16 height) {

    cached_swapchain_fb_width = width;
    cached_swapchain_fb_height = height;

    ++context.swapchain.framebuffer_size_generation;

    CORE_INFO("Vulkan renderer backend->resized: w/h/gen: %i %i %llu",
        width,
        height,
        context.swapchain.framebuffer_size_generation);

    // Notify UI module of resize
    // ui_on_vulkan_resize(&context, width, height);  // Commented out for UI
    // rewrite
}

b8 vulkan_begin_frame(Renderer_Backend* backend, f32 delta_t) {

    context.frame_delta_time = delta_t;

    Vulkan_Device* device = &context.device;

    if (context.recreating_swapchain) {
        // TODO: Blocking operation. To be optimized
        VkResult result = vkDeviceWaitIdle(device->logical_device);

        if (!vulkan_result_is_success(result)) {
            CORE_ERROR("vulkan_begin_frame vkDeviceWaitIdle (1) failed: '%s'",
                vulkan_result_string(result, true));
            return false;
        }

        CORE_INFO("Recreating swapchain, booting.");
        return false;
    }

    // Check if the framebuffer has been resized. If so, a new swapchain
    // must be created and since we will be creating a new swapchain object,
    // we cannot draw a frame image during this frame iteration
    // NOTE:	In renderer_frontend is the begin_frame function returns false
    // 			then the frame is not drawn
    if (context.swapchain.framebuffer_size_generation !=
        context.swapchain.framebuffer_size_last_generation) {

        VkResult result = vkDeviceWaitIdle(device->logical_device);
        if (!vulkan_result_is_success(result)) {
            CORE_ERROR("vulkan_begin_frame vkDeviceWaitIdle (2) failed: '%s'",
                vulkan_result_string(result, true));
            return false;
        }

        // If the swapchain recreationg failed (because the windows was
        // minimized) boot out before unsetting the flag
        if (!recreate_swapchain(backend, true)) {
            return false;
        }

        CORE_INFO("Resized, booting.");
        return false;
    }

    // Wait for the execution of the current frame to complete. The frence being
    // free will allow this one to move on
    VkResult result = vkWaitForFences(context.device.logical_device,
        1,
        &context.in_flight_fences[context.current_frame],
        true,
        UINT64_MAX);
    if (!vulkan_result_is_success(result)) {

        CORE_ERROR("In-flight fence wait failure with error: '%s'",
            vulkan_result_string(result, true));
        return false;
    }

    // Acquire the next image from the swapchain. Pass along the samephore that
    // should be signaled when this operation completes. This same semaphore
    // will later be waited on by the queue submission to ensure this image is
    // available
    if (!get_next_image_index(backend))
        return false;

    // CORE_DEBUG("frame_render() with frame: '%d' and image index: '%d'",
    //              context.current_frame, context.image_index);

    // At this point we have an image index that we can render to!

    // Begin recording commands
    Vulkan_Command_Buffer* cmd_buffer =
        &context.command_buffers[context.image_index];

    vulkan_command_buffer_reset(cmd_buffer);
    // Mark this command buffer NOT as single use since we are using this over
    // and over again
    vulkan_command_buffer_begin(cmd_buffer, false, false, false);

    VkViewport viewport;
    // The default viewport of vulkan starts at the top-left corner of the
    // viewport rectangle so coordinates (0; height) instead of (0;0) like in
    // OpenGL. In order to have consistency with other graphics APIs later on,
    // we can offset this.
    viewport.x = 0.0f;
    viewport.y = (f32)context.swapchain.framebuffer_height;
    viewport.width = (f32)context.swapchain.framebuffer_width;
    viewport.height = -(f32)context.swapchain.framebuffer_height;

    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Scissor (basically a Box) clips the scene to the size of the screen
    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = context.swapchain.framebuffer_width;
    scissor.extent.height = context.swapchain.framebuffer_height;

    vkCmdSetViewport(cmd_buffer->handle, 0, 1, &viewport);
    vkCmdSetScissor(cmd_buffer->handle, 0, 1, &scissor);

    context.ui_renderpass.render_area.z = context.swapchain.framebuffer_width;
    context.ui_renderpass.render_area.w = context.swapchain.framebuffer_height;

    return true;
}

void vulkan_update_global_viewport_state(mat4 projection,
    mat4 view,
    vec3 view_position,
    vec4 ambient_colour,
    s32 mode) {
    Vulkan_Command_Buffer* cmd_buffer =
        &context.command_buffers[context.image_index];

    // Bind pipeline
    vulkan_material_shader_pipeline_use(&context, &context.material_shader);

    // Update uniform buffer data
    context.material_shader.global_ubo.projection = projection;
    context.material_shader.global_ubo.view = view;

    // Bind descriptor sets
    vulkan_material_shader_pipeline_update_global_state(&context,
        &context.material_shader,
        context.frame_delta_time);
}

b8 vulkan_end_frame(Renderer_Backend* backend, f32 delta_t) {

    // First, finish recording and submit the viewport command buffer
    Vulkan_Command_Buffer* cmd_buffer =
        &context.command_buffers[context.image_index];

    // End command buffer recording
    vulkan_command_buffer_end(cmd_buffer);

    // Make sure the previous frame is not using this image (i.e. its fence is
    // being waited on)
    if (context.images_in_flight[context.image_index] != nullptr) {
        VkResult result = vkWaitForFences(context.device.logical_device,
            1,
            context.images_in_flight[context.image_index],
            true,
            UINT64_MAX);

        if (!vulkan_result_is_success(result)) {
            CORE_FATAL("vk_fence_wait error: '%s'",
                vulkan_result_string(result, true));
        }

        // by the time this operation completes, we are safe to perform ops.
    }

    // Mark the image fence as in-se by the current frame
    context.images_in_flight[context.image_index] =
        &context.in_flight_fences[context.current_frame];

    VK_CHECK(vkResetFences(context.device.logical_device,
        1,
        &context.in_flight_fences[context.current_frame]));

    // submit the queue and wait for the operation to complete
    // Begin queue submission
    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buffer->handle;

    // Semaphores to be signaled when the queue is complete
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores =
        &context.render_finished_semaphores[context.image_index];

    // Wait semaphore ensures that the operation cannot begin until the image
    // is available.
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores =
        &context.image_available_semaphores[context.current_frame];

    // Wait destination stage mask. PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    // which basically prevents the color attachment writes from executing
    // until the semaphore signals. Basically this means that only ONE frame is
    // presented

    VkPipelineStageFlags flags[1] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.pWaitDstStageMask = flags;

    // All the commands that have been queued will be submitted for execution
    VkResult result =
        vkQueueSubmit(context.device.graphics_queue, // graphics operation
            1,
            &submit_info,
            context.in_flight_fences[context.current_frame]);

    if (result != VK_SUCCESS) {
        CORE_ERROR("vkQueueSubmit for ui failed with result: '%s'",
            vulkan_result_string(result, true));
        return false;
    }

    vulkan_command_buffer_update_submitted(cmd_buffer);

    // Last stage is presentation
    if (!present_frame(backend))
        return false;

    return true;
}

// TODO: Change name
b8 vulkan_renderpass_start(Renderer_Backend* backend,
    Renderpass_Type renderpass_type) {
    Vulkan_Renderpass* renderpass = nullptr;

    VkFramebuffer framebuffer = nullptr;
    Vulkan_Command_Buffer* cmd_buffer =
        &context.command_buffers[context.image_index];

    switch (renderpass_type) {
    case Renderpass_Type::VIEWPORT: {
        renderpass = &context.viewport_renderpass;
        framebuffer = context.viewport.framebuffers[context.image_index];

        break;
    }

    case Renderpass_Type::UI: {
        renderpass = &context.ui_renderpass;
        framebuffer = context.swapchain.framebuffers[context.image_index];

        break;
    }
    default:
        CORE_ERROR(
            "vulkan_renderpass_begin - Unknown begin renderpass instruction "
            "for renderpass id: %#02x",
            (u8)renderpass_type);
        return false;
    };

    vulkan_renderpass_begin(cmd_buffer, renderpass, framebuffer);

    switch (renderpass_type) {

    case Renderpass_Type::VIEWPORT:
        vulkan_material_shader_pipeline_use(&context, &context.material_shader);
        break;
    case Renderpass_Type::UI:
        // Commented out for UI rewrite - will be implemented in
        // vulkan_ui_backend vulkan_imgui_shader_pipeline_use(&context,
        // &context.imgui_shader);
        break;
    }

    return true;
}

// TODO: Change name
b8 vulkan_renderpass_finish(Renderer_Backend* backend,
    Renderpass_Type renderpass_type) {

    Vulkan_Renderpass* renderpass = nullptr;

    Vulkan_Command_Buffer* cmd_buffer =
        &context.command_buffers[context.image_index];

    switch (renderpass_type) {
    case Renderpass_Type::VIEWPORT:
        renderpass = &context.viewport_renderpass;
        break;
    case Renderpass_Type::UI:
        renderpass = &context.ui_renderpass;
        break;
    default:
        CORE_ERROR(
            "vulkan_renderpass_end - Unknown begin renderpass instruction "
            "for renderpass id: %#02x",
            (u8)renderpass_type);
        return false;
    };

    vulkan_renderpass_end(cmd_buffer, renderpass);
    return true;
}

// (TODO) move the check of availability of the required layers outside
// this function
b8 vulkan_enable_validation_layers(
    Auto_Array<const char*>* required_layers_array) {

    CORE_INFO("Vulkan validation layers enabled. Enumerating...");

    // Declare the list of layers that we require
    required_layers_array->push_back("VK_LAYER_KHRONOS_validation");

    // Need to check whether the validation layer requuested is supported
    u32 available_layer_count;

    // To allocate the array needed for storing the available layers,
    // first I need to know how many layers there are
    VK_CHECK(
        vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr));

    Auto_Array<VkLayerProperties> available_layers_array;
    available_layers_array.resize(available_layer_count);

    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count,
        available_layers_array.data));

    for (u32 i = 0; i < required_layers_array->length; ++i) {
        CORE_INFO("Searching for layer: %s ...",
            required_layers_array->data[i]);

        b8 found = false;
        for (u32 j = 0; j < available_layer_count; ++j) {
            if (string_check_equal(required_layers_array->data[i],
                    available_layers_array[j].layerName)) {
                found = true;
                CORE_INFO("Found.");
                break;
            }
        }

        if (!found) {
            CORE_FATAL("Required validation layer is missing: %s",
                required_layers_array->data[i]);
            return false;
        }
    }

    CORE_INFO("All required validaton layers are valid");
    return true;
}

b8 vulkan_create_debug_logger(VkInstance* instance) {

    CORE_DEBUG("Creating Vulkan debug logger");

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};

    // Specify the level of events that we want to capture
    debug_create_info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;

    // Specify the nature of events that we want to be fed from the validation
    // layer
    debug_create_info.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

    debug_create_info.pfnUserCallback = vk_debug_callback;

    // Optional pointer that can be passed to the logger. Essentially we can
    // pass whatever data we want and use it in the callback function. Not used
    debug_create_info.pUserData = nullptr;

    // The vkCreateDebugUtilsMessengerEXT is an extension function so it is not
    // loaded automatically. Its address must be looked up manually
    VK_INSTANCE_LEVEL_FUNCTION(*instance, vkCreateDebugUtilsMessengerEXT);

    VK_CHECK(vkCreateDebugUtilsMessengerEXT(*instance,
        &debug_create_info,
        context.allocator,
        &context.debug_messenger));

    CORE_DEBUG("Vulkan debugger created");
    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {

    switch (message_severity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        CORE_ERROR(callback_data->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        CORE_WARN(callback_data->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        CORE_INFO(callback_data->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        CORE_TRACE(callback_data->pMessage);
        break;
    default:
        break;
    }
    return VK_FALSE;
}

s32 find_memory_index(u32 type_filter, u32 requested_property_flags) {

    VkPhysicalDeviceMemoryProperties memory_properties;

    // DeviceMemoryProperties structure contains the poroperties of both the
    // device's heaps and its supported memory types. The structure has the
    // memoryTypes[VK_MAX_MEMORY_TYPES] field which is an array of these
    // structures:
    //
    // VkMemoryType {
    // 		VkMemoryPropertyFlags	property_flags;
    // 		uint32_t				heapIndex;
    // }
    //
    // The flags field described the type of memory and is made of a combination
    // of the VkMemoryPropertyFlagBits flags. When creating a Vulka image, the
    // image itself specifies the type of memory it needs on the device in order
    // to be created, so we need to check whether that memory type is supported
    // and if so, we need to return the heapIndex to that memory

    vkGetPhysicalDeviceMemoryProperties(context.device.physical_device,
        &memory_properties);

    for (u32 i = 0; i < memory_properties.memoryTypeCount; ++i) {
        if (
            // Check if memory type i is acceptable according to the type_filter
            // we get from the memory requirements of the image.
            type_filter & (1 << i) &&
            // Check if the memory type i supports all required properties
            // (flags)
            (memory_properties.memoryTypes[i].propertyFlags &
                requested_property_flags) == requested_property_flags) {
            return i;
        }
    }

    CORE_WARN("Memory type not suitable");
    return -1;
}

void create_command_buffers(Vulkan_Context* context) {
    // Create command buffers for main renderer off-screen rendering
    // Match swapchain image_count for synchronization with image_index
    if (!context->command_buffers.data) {
        context->command_buffers.resize(context->swapchain.image_count);
        for (u32 i = 0; i < context->swapchain.image_count; ++i) {
            memory_zero(&context->command_buffers[i],
                sizeof(Vulkan_Command_Buffer));
        }
    }

    for (u32 i = 0; i < context->swapchain.image_count; ++i) {
        if (context->command_buffers[i].handle) {
            vulkan_command_buffer_free(context,
                context->device.graphics_command_pool,
                &context->command_buffers[i]);
        }

        memory_zero(&context->command_buffers[i],
            sizeof(Vulkan_Command_Buffer));

        vulkan_command_buffer_allocate(context,
            context->device.graphics_command_pool,
            true,
            &context->command_buffers[i]);
    }

    CORE_DEBUG("Command buffers created (count=%u)",
        context->swapchain.image_count);
}

// We need a framebuffer per swapchain image
void regenerate_framebuffers() {
    u32 image_count = context.swapchain.image_count;

    for (u32 i = 0; i < image_count; ++i) {

        VkImageView viewport_attachments[2] = {
            context.viewport.color_attachments[i].view,
            context.viewport.depth_attachment.view};

        VkFramebufferCreateInfo framebuffer_create_info = {
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebuffer_create_info.renderPass = context.viewport_renderpass.handle;
        framebuffer_create_info.attachmentCount = 2;
        framebuffer_create_info.pAttachments = viewport_attachments;
        framebuffer_create_info.width = context.viewport.framebuffer_width;
        framebuffer_create_info.height = context.viewport.framebuffer_height;
        framebuffer_create_info.layers = 1;

        VK_CHECK(vkCreateFramebuffer(context.device.logical_device,
            &framebuffer_create_info,
            context.allocator,
            &context.viewport.framebuffers[i]));

        VkImageView ui_attachments[2] = {context.swapchain.views[i]};

        VkFramebufferCreateInfo ui_framebuffer_create_info = {
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebuffer_create_info.renderPass = context.ui_renderpass.handle;
        framebuffer_create_info.attachmentCount = 1;
        framebuffer_create_info.pAttachments = ui_attachments;
        framebuffer_create_info.width = context.swapchain.framebuffer_width;
        framebuffer_create_info.height = context.swapchain.framebuffer_height;
        framebuffer_create_info.layers = 1;

        VK_CHECK(vkCreateFramebuffer(context.device.logical_device,
            &framebuffer_create_info,
            context.allocator,
            &context.swapchain.framebuffers[i]));
    }
}

b8 recreate_swapchain(Renderer_Backend* backend, b8 is_resized_event) {
    if (context.recreating_swapchain) {
        CORE_DEBUG(
            "recreate_swapchain called when already recreating. Booting.");
        return false;
    }

    if (context.swapchain.framebuffer_width == 0 ||
        context.swapchain.framebuffer_height == 0) {
        CORE_DEBUG(
            "recreate_swapchain called when window is <1 in a dimension. "
            "Booting.");
        return false;
    }

    const char* reason =
        is_resized_event ? "resize event" : "non-optimal result";
    CORE_INFO("Recreating swapchain (%s)", reason);

    // Mark as recreating if the dimensions are VALID
    context.recreating_swapchain = true;

    vkDeviceWaitIdle(context.device.logical_device);

    // For saferty, clear these
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {
        context.images_in_flight[i] = nullptr;
    }

    // Requery support
    vulkan_device_query_swapchain_capabilities(context.device.physical_device,
        context.surface,
        &context.device.swapchain_info);
    vulkan_device_detect_depth_format(&context.device);

    vulkan_swapchain_recreate(&context,
        cached_swapchain_fb_width,
        cached_swapchain_fb_height,
        &context.swapchain);

    // Sync the framebuffer size with the cached values, if the size has changed
    if (is_resized_event) {

        // We will have new cached framebuffer sized only if the on_resized
        // event was called, otherwise we need to just recreate the swapchain
        // due to not optimal results of the present or get_next_image operation
        // of the swapchain

        // Ideally we would just want to recreate the swapchain with the new
        // dimension coming from an event of xcb. The problem with that is that
        // the XCB events are asynchronous and can arrive before the Vulkan
        // surface has fully updated internally. This means that although we
        // request a specific width and height, the bounds of the extent could
        // truncate such values, so we must consider the dimensions of the
        // created swapchain (after being truncated with the allowed bounds)
        // instead of the values that we wanted, to prevent inconsistencies
        // context.framebuffer_width = cached_framebuffer_width;
        // context.framebuffer_height = cached_framebuffer_height;

        // Overwrite the framebuffer dimensions to be equal to the swapchain
        context.swapchain.framebuffer_width = context.swapchain.extent.width;
        context.swapchain.framebuffer_height = context.swapchain.extent.height;

        context.ui_renderpass.render_area.z =
            context.swapchain.framebuffer_width;

        context.ui_renderpass.render_area.w =
            context.swapchain.framebuffer_height;

        cached_swapchain_fb_width = 0;
        cached_swapchain_fb_height = 0;

        context.swapchain.framebuffer_size_last_generation =
            context.swapchain.framebuffer_size_generation;
    }

    // Cleanup command buffers
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {
        vulkan_command_buffer_free(&context,
            context.device.graphics_command_pool,
            &context.command_buffers[i]);
    }

    // Destroy framebuffers
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {
        vkDestroyFramebuffer(context.device.logical_device,
            context.viewport.framebuffers[i],
            context.allocator);

        vkDestroyFramebuffer(context.device.logical_device,
            context.swapchain.framebuffers[i],
            context.allocator);
    }

    context.ui_renderpass.render_area.z = context.swapchain.framebuffer_width;
    context.ui_renderpass.render_area.w = context.swapchain.framebuffer_height;
    context.ui_renderpass.render_area.x = 0;
    context.ui_renderpass.render_area.y = 0;

    regenerate_framebuffers();

    // Recreate main renderer command buffers
    create_command_buffers(&context);

    context.recreating_swapchain = false;

    CORE_DEBUG("recreate_swapchain completed all operations.");

    return true;
}

b8 get_next_image_index(Renderer_Backend* backend) {

    VkResult result = vkAcquireNextImageKHR(context.device.logical_device,
        context.swapchain.handle,
        UINT64_MAX,
        context.image_available_semaphores[context.current_frame],
        nullptr,
        &context.image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // if (!recreate_swapchain(backend, false))
        //     CORE_FATAL("get_next_image_index failed to recreate swapchain,
        //     due to out-of-date error result");

        return false;

    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        CORE_FATAL("Failed to acquire swapchain iamge!");
        return false;
    }

    return true;
}

b8 present_frame(Renderer_Backend* backend) {

    VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores =
        &context.render_finished_semaphores[context.image_index];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &context.swapchain.handle;
    present_info.pImageIndices = &context.image_index;
    present_info.pResults = 0;

    VkResult result =
        vkQueuePresentKHR(context.device.presentation_queue, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {

        // if (!recreate_swapchain(backend, false))
        //     CORE_FATAL("present_frame failed to recreate swapchain after
        //     presentation, due to suboptimal error code");

        // CORE_DEBUG("Swapchain recreated because presentation returned out
        // of date");

    } else if (result != VK_SUCCESS) {
        CORE_FATAL("Failed to present swap chain image!");
        return false;
    }

    context.current_frame =
        (context.current_frame + 1) % context.swapchain.max_in_flight_frames;

    return true;
}

INTERNAL_FUNC b8 create_buffers(Vulkan_Context* context) {

    // When using device local memory, it means that this memory will not be
    // accesable from the host CPU, however we can copy from or copy to this
    // buffer with/from other buffers. That is why the flag of TRANSFER_DST and
    // SRC is set when creating, so that temporary buffers with the wanted data
    // can be created, and subsequently their data can be copied into these
    // buffers
    VkMemoryPropertyFlagBits memory_property_flags =
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    constexpr u64 vertex_buffer_size = sizeof(vertex_3d) * 1024 * 1024; // 64mb

    if (!vulkan_buffer_create(context,
            vertex_buffer_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            memory_property_flags,
            true,
            &context->object_vertex_buffer)) {
        CORE_ERROR("Error creating vertex buffer.");
        return false;
    }

    context->geometry_vertex_offset = 0;
    CORE_INFO("Created vertex buffer");

    constexpr u64 index_buffer_size = sizeof(u32) * 1024 * 1024; // 64mb

    if (!vulkan_buffer_create(context,
            index_buffer_size,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            memory_property_flags,
            true,
            &context->object_index_buffer)) {
        CORE_ERROR("Error creating vertex buffer.");
        return false;
    }

    CORE_INFO("Created index buffer");

    context->geometry_index_offset = 0;

    return true;
}

void vulkan_create_texture(const u8* pixels, Texture* texture) {

    // Internal data creation
    // TODO: Use a custom allocator for this
    texture->internal_data =
        memory_allocate(sizeof(Vulkan_Texture_Data), Memory_Tag::TEXTURE);

    Vulkan_Texture_Data* data =
        static_cast<Vulkan_Texture_Data*>(texture->internal_data);

    VkDeviceSize image_size =
        texture->width * texture->height * texture->channel_count;

    // NOTE: Assume 8 bits per channel
    VkFormat image_format = VK_FORMAT_R8G8B8A8_UNORM;

    // Create a staging buffer and load data into it
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags memory_prop_flags =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    Vulkan_Buffer staging;
    vulkan_buffer_create(&context,
        image_size,
        usage,
        memory_prop_flags,
        true,
        &staging);

    vulkan_buffer_load_data(&context, &staging, 0, image_size, 0, pixels);

    // Assume the image type is 2D
    vulkan_image_create(&context,
        VK_IMAGE_TYPE_2D,
        texture->width,
        texture->height,
        image_format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        true,
        VK_IMAGE_ASPECT_COLOR_BIT,
        &data->image);

    // Load the data of the buffer into the image. To load data into the image
    // from the buffer we need to use a command buffer
    Vulkan_Command_Buffer temp_buffer;
    VkCommandPool pool = context.device.graphics_command_pool;
    VkQueue queue = context.device.graphics_queue;
    vulkan_command_buffer_startup_single_use(&context, pool, &temp_buffer);

    vulkan_image_transition_layout(&context,
        &temp_buffer,
        &data->image,
        image_format,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    vulkan_image_copy_from_buffer(&context,
        &data->image,
        staging.handle,
        &temp_buffer);

    vulkan_image_transition_layout(&context,
        &temp_buffer,
        &data->image,
        image_format,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vulkan_command_buffer_end_single_use(&context, pool, &temp_buffer, queue);

    // Destroy staging buffer AFTER command buffer has been submitted and
    // completed
    vulkan_buffer_destroy(&context, &staging);

    VkSamplerCreateInfo sampler_info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = 16;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;

    VkResult result = vkCreateSampler(context.device.logical_device,
        &sampler_info,
        context.allocator,
        &data->sampler);

    if (!vulkan_result_is_success(result)) {
        CORE_ERROR("Error creating texture sampler: '%s'",
            vulkan_result_string(result, true));
        return;
    }

    texture->generation++;
}

void vulkan_destroy_texture(Texture* texture) {
    vkDeviceWaitIdle(context.device.logical_device);

    Vulkan_Texture_Data* data =
        static_cast<Vulkan_Texture_Data*>(texture->internal_data);

    if (data) {
        vulkan_image_destroy(&context, &data->image);
        memory_zero(&data->image, sizeof(Vulkan_Image));
        vkDestroySampler(context.device.logical_device,
            data->sampler,
            context.allocator);
        data->sampler = nullptr;

        memory_deallocate(texture->internal_data,
            sizeof(Vulkan_Texture_Data),
            Memory_Tag::TEXTURE);
    }

    memory_zero(texture, sizeof(struct Texture));
}

b8 vulkan_create_material(struct Material* material) {
    if (material) {
        if (!vulkan_material_shader_pipeline_acquire_resource(&context,
                &context.material_shader,
                material)) {
            CORE_ERROR(
                "vulkan_renderer_create_material - Failed to acquire shader "
                "resources");
            return false;
        }

        CORE_TRACE("Renderer: Material created.");
        return true;
    }

    return false;
}

void vulkan_destroy_material(struct Material* material) {
    if (material) {
        if (material->internal_id != INVALID_ID) {
            vulkan_material_shader_pipeline_release_resource(&context,
                &context.material_shader,
                material);
        } else {
            CORE_WARN(
                "vulkan_destroy_material called with internal_id = INVALID_ID. "
                "Nothing was done");
        }
    } else {
        CORE_WARN("vulkan_destroy_material called with nullptr");
    }
}

b8 vulkan_create_geometry(Geometry* geometry,
    u32 vertex_count,
    const vertex_3d* vertices,
    u32 index_count,
    const u32* indices) {
    if (!vertex_count || !vertices) {
        CORE_ERROR(
            "vulkan_create_geometry requires vert data and none was provided, "
            "vertex_count=%d, vertices=%p",
            vertex_count,
            vertices);

        return false;
    }

    // Check if this geometry is a reupload. If yes, old data must be freed
    b8 is_reupload = geometry->internal_id != INVALID_ID;
    Vulkan_Geometry_Data old_range;

    Vulkan_Geometry_Data* internal_data = nullptr;

    if (is_reupload) {
        internal_data = &context.registered_geometries[geometry->internal_id];

        old_range.index_buffer_offset = internal_data->index_buffer_offset;
        old_range.index_count = internal_data->index_count;
        old_range.index_size = internal_data->index_size;
        old_range.vertex_buffer_offset = internal_data->vertex_buffer_offset;
        old_range.vertex_count = internal_data->vertex_count;
        old_range.vertex_size = internal_data->vertex_size;
    } else {
        for (u32 i = 0; i < VULKAN_MAX_GEOMETRY_COUNT; ++i) {
            if (context.registered_geometries[i].id == INVALID_ID) {
                geometry->internal_id = i;
                context.registered_geometries[i].id = i;
                internal_data = &context.registered_geometries[i];
                break;
            }
        }
    }

    if (!internal_data) {
        CORE_FATAL(
            "vulkan_renderer_create_geometry failed to find a free index for a "
            "new geometry upload. Change config to increase max geometry size");
        return false;
    }

    VkCommandPool pool = context.device.graphics_command_pool;
    VkQueue queue = context.device.graphics_queue;

    internal_data->vertex_buffer_offset = context.geometry_vertex_offset;
    internal_data->vertex_count = vertex_count;
    internal_data->vertex_size = sizeof(vertex_3d) * vertex_count;

    upload_data_range(&context,
        pool,
        0,
        queue,
        &context.object_vertex_buffer,
        internal_data->vertex_buffer_offset,
        internal_data->vertex_size,
        vertices);

    context.geometry_vertex_offset += internal_data->vertex_size;

    // It is possible to handle a geometry that does not have index data
    if (index_count && indices) {
        internal_data->index_buffer_offset = context.geometry_index_offset;
        internal_data->index_count = index_count;
        internal_data->index_size = sizeof(u32) * index_count;

        upload_data_range(&context,
            pool,
            0,
            queue,
            &context.object_index_buffer,
            internal_data->index_buffer_offset,
            internal_data->index_size,
            indices);

        context.geometry_index_offset += internal_data->index_size;
    }

    if (internal_data->generation == INVALID_ID) {
        internal_data->generation = 0;
    } else {
        internal_data->generation++;
    }

    if (is_reupload) {
        free_data_range(&context.object_vertex_buffer,
            old_range.vertex_buffer_offset,
            old_range.vertex_size);

        if (old_range.index_size > 0) {
            free_data_range(&context.object_index_buffer,
                old_range.index_buffer_offset,
                old_range.index_size);
        }
    }

    return true;
}

void vulkan_destroy_geometry(Geometry* geometry) {
    if (geometry && geometry->internal_id != INVALID_ID) {
        vkDeviceWaitIdle(context.device.logical_device);

        Vulkan_Geometry_Data* internal_data =
            &context.registered_geometries[geometry->internal_id];

        free_data_range(&context.object_vertex_buffer,
            internal_data->vertex_buffer_offset,
            internal_data->vertex_size);

        if (internal_data->index_size > 0) {
            free_data_range(&context.object_index_buffer,
                internal_data->index_buffer_offset,
                internal_data->index_size);
        }

        memory_zero(internal_data, sizeof(Vulkan_Geometry_Data));
        internal_data->id = INVALID_ID;
        internal_data->generation = INVALID_ID;
    }
}

void vulkan_draw_geometry(Geometry_Render_Data data) {
    if (!data.geometry || data.geometry->internal_id == INVALID_ID) {
        return;
    }

    Vulkan_Geometry_Data* buffer_data =
        &context.registered_geometries[data.geometry->internal_id];

    Vulkan_Command_Buffer* cmd_buffer =
        &context.command_buffers[context.image_index];

    // TODO: Check if this is needed
    vulkan_material_shader_pipeline_use(&context, &context.material_shader);

    vulkan_material_shader_pipeline_set_model(&context,
        &context.material_shader,
        data.model);

    if (data.geometry->material) {
        vulkan_material_shader_pipeline_apply_material(&context,
            &context.material_shader,
            data.geometry->material);
    } else {
        vulkan_material_shader_pipeline_apply_material(&context,
            &context.material_shader,
            material_system_get_default());
    }

    // Bind vertex and index buffers
    VkDeviceSize offsets[1] = {buffer_data->vertex_buffer_offset};
    vkCmdBindVertexBuffers(cmd_buffer->handle,
        0,
        1,
        &context.object_vertex_buffer.handle,
        offsets);

    if (buffer_data->index_count > 0) {

        vkCmdBindIndexBuffer(cmd_buffer->handle,
            context.object_index_buffer.handle,
            buffer_data->index_buffer_offset,
            VK_INDEX_TYPE_UINT32);

        // Issue the draw
        vkCmdDrawIndexed(cmd_buffer->handle,
            buffer_data->index_count,
            1,
            0,
            0,
            0);
    } else {
        vkCmdDraw(cmd_buffer->handle, buffer_data->vertex_count, 1, 0, 0);
    }
}

void vulkan_draw_ui(UI_Render_Data data) {
    vulkan_imgui_shader_pipeline_draw(&context,
        &context.imgui_shader,
        data.draw_list);
}
