#include "vulkan_command_buffer.hpp"
#include "core/logger.hpp"
#include "memory/memory.hpp"

void vulkan_command_buffer_allocate(Vulkan_Context *context,
    VkCommandPool pool,
    b8 is_primary,
    Vulkan_Command_Buffer *out_command_buffer) {

    memory_zero(out_command_buffer, sizeof(Vulkan_Command_Buffer));

    VkCommandBufferAllocateInfo allocate_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};

    allocate_info.commandPool = pool;
    allocate_info.level = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY
                                     : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocate_info.commandBufferCount = 1;
    allocate_info.pNext = nullptr;

    out_command_buffer->state = Command_Buffer_State::NOT_ALLOCATED;

    // Technically the command_buffers input of the allocate function needs an
    // array of command buffers, so it should be a double pointer, but since we
    // have only one command buffer (for now) we can pass the address of the
    // single command buffer
    VK_CHECK(vkAllocateCommandBuffers(context->device.logical_device,
        &allocate_info,
        &out_command_buffer->handle));

    out_command_buffer->state = Command_Buffer_State::READY;
}

void vulkan_command_buffer_free(Vulkan_Context *context,
    VkCommandPool pool,
    Vulkan_Command_Buffer *command_buffer) {

    vkFreeCommandBuffers(context->device.logical_device,
        pool,
        1,
        &command_buffer->handle);

    command_buffer->handle = nullptr;
    command_buffer->state = Command_Buffer_State::NOT_ALLOCATED;
}

void vulkan_command_buffer_begin(Vulkan_Command_Buffer *command_buffer,
    b8 is_single_use,
    b8 is_renderpass_continue,
    b8 is_simultaneous_use) {

    VkCommandBufferBeginInfo begin_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

    begin_info.flags = 0;
    if (is_single_use) {
        // Specifies that each recording of the command bffer will only be
        // submitted once, and the command buffer will be reset and recorded
        // again between each submission
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    }
    if (is_renderpass_continue) {
        // Specifies that a secondary command buffer is considered to be
        // entirely inside the render pass. If this is a primary command buffer
        // this bit is ignored
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    }
    if (is_simultaneous_use) {
        // Specifies that a command buffer can be resubmitted to a queue while
        // it is in the pending state and recorded into multiple primary command
        // buffers
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    }

    VK_CHECK(vkBeginCommandBuffer(command_buffer->handle, &begin_info));

    command_buffer->state = Command_Buffer_State::RECORDING;
}

void vulkan_command_buffer_end(Vulkan_Command_Buffer *command_buffer) {

    VK_CHECK(vkEndCommandBuffer(command_buffer->handle));

    command_buffer->state = Command_Buffer_State::RECORDING_ENDED;
}

void vulkan_command_buffer_update_submitted(
    Vulkan_Command_Buffer *command_buffer) {
    command_buffer->state = Command_Buffer_State::SUBMITTED;
}

void vulkan_command_buffer_reset(Vulkan_Command_Buffer *command_buffer) {

    // Reset the Vulkan command buffer handle to INITIAL state
    // This must be called after the fence wait (which happens in
    // vulkan_begin_frame)
    VK_CHECK(vkResetCommandBuffer(command_buffer->handle, 0));

    command_buffer->state = Command_Buffer_State::READY;
}

// Allocates and begins a single use command buffer
void vulkan_command_buffer_startup_single_use(Vulkan_Context *context,
    VkCommandPool pool,
    Vulkan_Command_Buffer *out_command_buffer) {

    vulkan_command_buffer_allocate(context, pool, true, out_command_buffer);

    vulkan_command_buffer_begin(out_command_buffer, true, false, false);
}

void vulkan_command_buffer_end_single_use(Vulkan_Context *context,
    VkCommandPool pool,
    Vulkan_Command_Buffer *command_buffer,
    VkQueue queue) {

    vulkan_command_buffer_end(command_buffer);

    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer->handle;

    VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, 0));

    // Technically a fence should be used to properly wait for the submit
    // however we can wait for it to finish synchronously
    VK_CHECK(vkQueueWaitIdle(queue));

    // After we wait, we can safelly free the command buffer
    vulkan_command_buffer_free(context, pool, command_buffer);
}
