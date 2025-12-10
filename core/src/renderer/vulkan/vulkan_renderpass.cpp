#include "vulkan_renderpass.hpp"
#include "core/logger.hpp"
#include "memory/memory.hpp"
#include "renderer/vulkan/vulkan_types.hpp"

void vulkan_renderpass_create(Vulkan_Context* context,
    Vulkan_Renderpass* out_renderpass,
    vec4 render_area,
    vec4 clear_color,
    f32 depth,
    u32 stencil,
    Renderpass_Clear_Flags clear_flags,
    b8 has_prev_pass,
    b8 has_next_pass) {

    out_renderpass->render_area = render_area;
    out_renderpass->clear_color = clear_color;
    out_renderpass->clear_flags = (u8)clear_flags;

    out_renderpass->has_prev = has_prev_pass;
    out_renderpass->has_next = has_next_pass;

    out_renderpass->depth = depth;
    out_renderpass->stencil = stencil;

    // Main subpass - Graphics pipeline. For the moment the renderpass will
    // have only 1 pass
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    // Prepare two attachments for color and depth
    u32 attachment_description_count = 0;
    VkAttachmentDescription attachment_descriptions[2];

    // Determine for the color attachment whether we will be clearing the color
    b8 do_clear_color =
        u32(clear_flags & Renderpass_Clear_Flags::COLOR_BUFFER) != 0;

    // We need to define what attachments will be used during rendering, i.e.
    // images like color/depth buffers
    VkAttachmentDescription color_attachment;
    // Use standard RGBA format for off-screen rendering
    color_attachment.format = VK_FORMAT_B8G8R8A8_UNORM;
    // Set each pixel to be samples only once by the shaders
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    // When the renderpass starts, clear the color attachment (typically to
    // black of specified color)
    color_attachment.loadOp = do_clear_color ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                             : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    // When the renderpass stops, store the results. This settings is very
    // important since we want to store the results of the renderpass to allow
    // the swapchain to present them
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // Since we do not use stencil operations, we do not care about these two
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // For viewport pass: after first frame, image stays in
    // SHADER_READ_ONLY_OPTIMAL Using same layout for initial and final avoids
    // unnecessary transitions
    color_attachment.initialLayout = has_next_pass
                                         ? VK_IMAGE_LAYOUT_UNDEFINED  // VP pass
                                         : VK_IMAGE_LAYOUT_UNDEFINED; // UI pass
    // After render pass, the image will be ready for shader sampling
    color_attachment.finalLayout =
        has_next_pass ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL // VP pass
                      : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;         // UI pass
    color_attachment.flags = 0;
    attachment_descriptions[attachment_description_count++] = color_attachment;

    // Specify that during the subpass. attachment with index 0 will be used
    // with color-optimzed layout
    VkAttachmentReference color_attachment_reference;
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout =
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Bind the color attachment to the subpass
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_reference;

    b8 do_clear_depth =
        u32(clear_flags & Renderpass_Clear_Flags::DEPTH_BUFFER) != 0;

    // Skip the depth attachment in case of the UI renderpass
    if (do_clear_depth) {
        VkAttachmentDescription depth_attachment = {};
        depth_attachment.format = context->device.depth_format;

        depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;

        depth_attachment.loadOp = do_clear_depth ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                                 : VK_ATTACHMENT_LOAD_OP_LOAD;
        // We do not vare about the z-buffer contents after rendering
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        // We do not do stencil operations
        depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.finalLayout =
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        attachment_descriptions[attachment_description_count++] =
            depth_attachment;

        VkAttachmentReference depth_attachment_reference;
        depth_attachment_reference.attachment = 1;
        depth_attachment_reference.layout =
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        subpass.pDepthStencilAttachment = &depth_attachment_reference;
    } else {
        memory_zero(&attachment_descriptions[attachment_description_count],
            sizeof(VkAttachmentDescription));
        subpass.pDepthStencilAttachment = nullptr;
    }

    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;

    subpass.pResolveAttachments = nullptr;

    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;

    VkSubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                               VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = 0;

    VkRenderPassCreateInfo create_info = {
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};

    // Specify the attachments
    create_info.attachmentCount = attachment_description_count;
    create_info.pAttachments = attachment_descriptions;

    // Define the subpasses. In this case only 1 subpass
    create_info.subpassCount = 1;
    create_info.pSubpasses = &subpass;

    // Dependecies
    create_info.dependencyCount = 1;
    create_info.pDependencies = &dependency;

    create_info.pNext = nullptr;
    create_info.flags = 0;

    VK_CHECK(vkCreateRenderPass(context->device.logical_device,
        &create_info,
        context->allocator,
        &out_renderpass->handle));

    CORE_INFO("Renderpass object created successfully");
}

void vulkan_renderpass_destroy(Vulkan_Context* context,
    Vulkan_Renderpass* renderpass) {

    if (renderpass && renderpass->handle) {
        vkDestroyRenderPass(context->device.logical_device,
            renderpass->handle,
            context->allocator);
        renderpass->handle = nullptr;
    }
}

void vulkan_renderpass_begin(Vulkan_Command_Buffer* command_buffer,
    Vulkan_Renderpass* renderpass,
    VkFramebuffer frame_buffer) {

    VkRenderPassBeginInfo begin_info = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};

    begin_info.renderPass = renderpass->handle;
    begin_info.framebuffer = frame_buffer;

    begin_info.renderArea.offset.x = renderpass->render_area.x;
    begin_info.renderArea.offset.y = renderpass->render_area.y;
    begin_info.renderArea.extent.width = renderpass->render_area.z;
    begin_info.renderArea.extent.height = renderpass->render_area.w;

    begin_info.clearValueCount = 0;
    begin_info.pClearValues = nullptr;

    VkClearValue clear_values[2];
    memory_zero(clear_values, sizeof(VkClearValue) * 2);

    b8 do_clear_color = (renderpass->clear_flags &
                            (u32)Renderpass_Clear_Flags::COLOR_BUFFER) != 0;
    if (do_clear_color) {
        memory_copy(clear_values[begin_info.clearValueCount].color.float32,
            renderpass->clear_color.elements,
            sizeof(f32) * 4);

        begin_info.clearValueCount++;
    }

    b8 do_clear_depth = (renderpass->clear_flags &
                            (u8)Renderpass_Clear_Flags::DEPTH_BUFFER) != 0;
    if (do_clear_depth) {
        memory_copy(clear_values[begin_info.clearValueCount].color.float32,
            renderpass->clear_color.elements,
            sizeof(f32) * 4);

        clear_values[begin_info.clearValueCount].depthStencil.depth =
            renderpass->depth;

        b8 do_clear_stencil =
            (renderpass->clear_flags &
                (u32)Renderpass_Clear_Flags::STENCIL_BUFFER) != 0;

        clear_values[begin_info.clearValueCount].depthStencil.stencil =
            do_clear_stencil ? renderpass->stencil : 0;

        begin_info.clearValueCount++;
    }

    begin_info.pClearValues = begin_info.clearValueCount > 0 ? clear_values : 0;

    vkCmdBeginRenderPass(command_buffer->handle,
        &begin_info,
        VK_SUBPASS_CONTENTS_INLINE);

    command_buffer->state = Command_Buffer_State::IN_RENDER_PASS;
}

void vulkan_renderpass_end(Vulkan_Command_Buffer* command_buffer,
    Vulkan_Renderpass* renderpass) {

    vkCmdEndRenderPass(command_buffer->handle);
    command_buffer->state = Command_Buffer_State::RECORDING;
}
