#pragma once

#include "vulkan_types.hpp"

void vulkan_command_buffer_allocate(Vulkan_Context *context,
    VkCommandPool pool,
    b8 is_primary,
    Vulkan_Command_Buffer *out_command_buffer);

void vulkan_command_buffer_free(Vulkan_Context *context,
    VkCommandPool pool,
    Vulkan_Command_Buffer *command_buffer);

void vulkan_command_buffer_begin(Vulkan_Command_Buffer *command_buffer,
    b8 is_single_use,
    b8 is_renderpass_continue,
    b8 is_simultaneous_use);

void vulkan_command_buffer_end(Vulkan_Command_Buffer *command_buffer);

void vulkan_command_buffer_update_submitted(
    Vulkan_Command_Buffer *command_buffer);

void vulkan_command_buffer_reset(Vulkan_Command_Buffer *command_buffer);

// Allocates and begins a single use command buffer
void vulkan_command_buffer_startup_single_use(Vulkan_Context *context,
    VkCommandPool pool,
    Vulkan_Command_Buffer *out_command_buffer);

void vulkan_command_buffer_end_single_use(Vulkan_Context *context,
    VkCommandPool pool,
    Vulkan_Command_Buffer *command_buffer,
    VkQueue queue);
