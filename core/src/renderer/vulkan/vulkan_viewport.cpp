#include "vulkan_viewport.hpp"
#include "core/logger.hpp"
#include "imgui_impl_vulkan.h"
#include "vulkan_command_buffer.hpp"
#include "vulkan_framebuffer.hpp"
#include "vulkan_image.hpp"
#include "vulkan_renderpass.hpp"
#include <cmath>

// Size tolerance for resize optimization (avoid micro-resizes during window
// dragging)
#define VIEWPORT_RESIZE_TOLERANCE 8

b8 vulkan_viewport_initialize(Vulkan_Context* context) {
    CORE_DEBUG("Initializing viewport rendering system...");

    // Initialize viewport render target with default size
    u32 default_width = 800;
    u32 default_height = 600;

    // Note: Main target resources are created in vulkan_backend.cpp
    // This function is now just for completeness

    // Note: Descriptor set creation is deferred until first use
    // because ImGui Vulkan backend may not be initialized yet

    CORE_INFO("Viewport rendering system initialized successfully");
    return true;
}

void vulkan_viewport_shutdown(Vulkan_Context* context) {
    CORE_DEBUG("Shutting down viewport rendering system...");

    // Note: Actual resource destruction is handled in vulkan_backend shutdown
    // This function is kept for API consistency

    CORE_DEBUG("Viewport rendering system shut down");
}

void vulkan_viewport_render(Vulkan_Context* context) {
    // Use image_index to synchronize with swapchain, matching UI renderpass
    // behavior Both the off-screen renderer and UI use the same image_index for
    // their respective framebuffers and command buffers
    Vulkan_Command_Buffer* main_command_buffer =
        &context->main_command_buffers[context->image_index];

    // Reset command buffer before recording (similar to UI command buffer)
    vulkan_command_buffer_reset(main_command_buffer);

    // Begin recording main renderer commands
    vulkan_command_buffer_begin(main_command_buffer, false, false, false);

    // Begin rendering to the main render target (use image_index's framebuffer)
    // The renderpass will automatically transition from
    // SHADER_READ_ONLY_OPTIMAL to COLOR_ATTACHMENT_OPTIMAL at the beginning,
    // and back to SHADER_READ_ONLY_OPTIMAL at the end (configured in renderpass
    // creation)
    vulkan_renderpass_begin(main_command_buffer,
        &context->main_renderpass,
        context->main_target.framebuffers[context->image_index].handle);

    // Set viewport for main render target
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (f32)context->main_target.width;
    viewport.height = (f32)context->main_target.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // CORE_DEBUG("Setting viewport: %ux%u", context->main_target.width,
    // context->main_target.height);
    vkCmdSetViewport(main_command_buffer->handle, 0, 1, &viewport);

    // Set scissor for main render target
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = {context->main_target.width, context->main_target.height};
    vkCmdSetScissor(main_command_buffer->handle, 0, 1, &scissor);

    // Command buffer remains in recording state
    // Drawing will be done in vulkan_update_global_state() after descriptor
    // sets are bound Submission will be done in vulkan_end_frame()
}

void vulkan_viewport_resize(Vulkan_Context* context, u32 width, u32 height) {

    // Apply size tolerance to avoid constant resizing during window dragging
    u32 current_width = context->main_target.width;
    u32 current_height = context->main_target.height;

    s32 width_diff = abs((s32)width - (s32)current_width);
    s32 height_diff = abs((s32)height - (s32)current_height);

    if (width_diff <= VIEWPORT_RESIZE_TOLERANCE &&
        height_diff <= VIEWPORT_RESIZE_TOLERANCE) {
        return; // Skip resize if change is too small
    }

    CORE_DEBUG("Resizing viewport from %ux%u to %ux%u",
        current_width,
        current_height,
        width,
        height);

    // Create sampler info (reused for all frames)
    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 1.0f;
    sampler_info.maxAnisotropy = 1.0f;

    // Destroy and recreate all framebuffer resources
    for (u32 i = 0; i < context->main_target.framebuffer_count; ++i) {
        // Destroy existing resources
        if (context->main_target.descriptor_sets[i] != VK_NULL_HANDLE) {
            ImGui_ImplVulkan_RemoveTexture(
                context->main_target.descriptor_sets[i]);
            context->main_target.descriptor_sets[i] = VK_NULL_HANDLE;
        }

        // Destroy sampler
        if (context->main_target.samplers[i] != VK_NULL_HANDLE) {
            vkDestroySampler(context->device.logical_device,
                context->main_target.samplers[i],
                context->allocator);
            context->main_target.samplers[i] = VK_NULL_HANDLE;
        }

        vulkan_framebuffer_destroy(context,
            &context->main_target.framebuffers[i]);
        vulkan_image_destroy(context,
            &context->main_target.color_attachments[i]);
        vulkan_image_destroy(context,
            &context->main_target.depth_attachments[i]);

        // Recreate with new size
        vulkan_image_create(context,
            VK_IMAGE_TYPE_2D,
            width,
            height,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            true,
            VK_IMAGE_ASPECT_COLOR_BIT,
            &context->main_target.color_attachments[i]);

        vulkan_image_create(context,
            VK_IMAGE_TYPE_2D,
            width,
            height,
            context->device.depth_format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            true,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            &context->main_target.depth_attachments[i]);

        VkImageView attachments_views[] = {
            context->main_target.color_attachments[i].view,
            context->main_target.depth_attachments[i].view};

        vulkan_framebuffer_create(context,
            &context->main_renderpass,
            width,
            height,
            2,
            attachments_views,
            &context->main_target.framebuffers[i]);

        // Create sampler
        VK_CHECK(vkCreateSampler(context->device.logical_device,
            &sampler_info,
            context->allocator,
            &context->main_target.samplers[i]));

        Vulkan_Command_Buffer command_buffer;
        vulkan_command_buffer_startup_single_use(context,
            context->device.graphics_command_pool,
            &command_buffer);

        // Transition color attachment to shader read-only layout
        vulkan_image_transition_layout(context,
            &command_buffer,
            &context->main_target.color_attachments[i],
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vulkan_command_buffer_end_single_use(context,
            context->device.graphics_command_pool,
            &command_buffer,
            context->device.graphics_queue);
    }

    CORE_DEBUG("Offscreen render targets resized successfully (count=%u)",
        context->main_target.framebuffer_count);

    context->main_target.width = width;
    context->main_target.height = height;

    // Update descriptor sets after resize
    vulkan_viewport_update_descriptor(context);
}

VkDescriptorSet vulkan_viewport_get_texture(Vulkan_Context* context) {
    // Lazily create descriptor sets if they don't exist
    // This handles the case where ImGui Vulkan backend wasn't ready during
    // initialization
    // Use image_index to synchronize with swapchain
    if (context->main_target.descriptor_sets[context->image_index] ==
        VK_NULL_HANDLE) {
        vulkan_viewport_update_descriptor(context);
    }
    return context->main_target.descriptor_sets[context->image_index];
}

void vulkan_viewport_update_descriptor(Vulkan_Context* context) {
    // Update descriptor sets for all framebuffers
    for (u32 i = 0; i < context->main_target.framebuffer_count; ++i) {
        // Remove existing descriptor set if it exists
        if (context->main_target.descriptor_sets[i] != VK_NULL_HANDLE) {
            ImGui_ImplVulkan_RemoveTexture(
                context->main_target.descriptor_sets[i]);
            context->main_target.descriptor_sets[i] = VK_NULL_HANDLE;
        }

        // Create ImGui descriptor set for displaying as texture
        context->main_target.descriptor_sets[i] =
            ImGui_ImplVulkan_AddTexture(context->main_target.samplers[i],
                context->main_target.color_attachments[i].view,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        if (context->main_target.descriptor_sets[i] == VK_NULL_HANDLE) {
            CORE_ERROR(
                "Failed to create ImGui descriptor set for viewport texture "
                "(frame %u)!",
                i);
        } else {
            CORE_DEBUG("Viewport descriptor set updated (frame %u): %p",
                i,
                (void*)context->main_target.descriptor_sets[i]);
        }
    }
}
