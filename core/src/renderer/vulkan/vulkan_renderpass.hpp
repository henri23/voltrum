#pragma once
#include "math/math_types.hpp"
#include "utils/enum.hpp"
#include "vulkan_types.hpp"

// The renderpass tells Vulkan about the framebuffer attachments, the images
// that we created that we are going to be using while rendering i.e. our color
// attachments and our depth attachments
// In complex applications, the screne is built over many passes, where each
// pass renders a specific part of the scene. As single renderpass object
// encapsulates multiple passes or rendering phases over a single set of output
// images. Each pass withing the renderpass is known as a subpass.
// All drawing must be contained inside a renderpass. The graphics pipeline
// needs to know where its rendering to, so the renderpass object must be
// created before the graphics pipeline so the renderpass can tell it about the
// images.

enum class Renderpass_Clear_Flags : u32 {
    NONE = 0x0,
    COLOR_BUFFER = 0x1,
    DEPTH_BUFFER = 0x2,
    STENCIL_BUFFER = 0x4,
};

ENABLE_BITMASK(Renderpass_Clear_Flags);

// Create renderpass based on type - dispatches to appropriate internal function
void vulkan_renderpass_create(Vulkan_Context *context,
    Vulkan_Renderpass *out_renderpass,
    vec4 render_area,
    vec4 clear_color,
    f32 depth,
    u32 stencil,
    Renderpass_Clear_Flags clear_flags,
    b8 has_prev_pass,
    b8 has_next_pass);

void vulkan_renderpass_destroy(Vulkan_Context *context,
    Vulkan_Renderpass *renderpass);

void vulkan_renderpass_begin(Vulkan_Command_Buffer *command_buffer,
    Vulkan_Renderpass *renderpass,
    VkFramebuffer frame_buffer);

void vulkan_renderpass_end(Vulkan_Command_Buffer *command_buffer,
    Vulkan_Renderpass *renderpass);
