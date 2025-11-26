#pragma once

#include "renderer/vulkan/vulkan_types.hpp"

/**
 * UI -> Renderer Interface
 *
 * This header defines functions that the UI module provides to the renderer.
 * The UI module owns its Vulkan resource setup and provides draw data to the
 * renderer.
 *
 * Implementation is in ui.cpp
 */

struct ImDrawData; // Forward declaration

/**
 * Initialize UI Vulkan resources
 * UI module takes ownership of setting up its own Vulkan resources
 * @param context - Vulkan context for resource creation
 * @return true if successful, false otherwise
 */
b8 ui_setup_vulkan_resources(Vulkan_Context* context);

/**
 * Cleanup UI Vulkan resources
 * @param context - Vulkan context for resource destruction
 */
void ui_cleanup_vulkan_resources(Vulkan_Context* context);

/**
 * Handle Vulkan swapchain resize
 * @param context - Vulkan context
 * @param width - New width
 * @param height - New height
 */
void ui_on_vulkan_resize(Vulkan_Context* context, u32 width, u32 height);

/**
 * Begin UI frame
 * Called by renderer at the start of UI rendering
 */
void ui_begin_vulkan_frame();

/**
 * Render UI components
 * Called by renderer to let UI generate its geometry
 */
b8 ui_draw_components(Vulkan_Command_Buffer* command_buffer);
