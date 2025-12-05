#include "vulkan_ui_backend.hpp"
#include "core/logger.hpp"
#include "renderer/vulkan/vulkan_platform.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan_core.h>

// Helper function to check Vulkan results
INTERNAL_FUNC void check_vk_result(VkResult err) {
    if (err == VK_SUCCESS)
        return;
    CORE_ERROR("[ImGui] Vulkan error: VkResult = %d", err);
    if (err < 0) {
        CORE_FATAL("[ImGui] Fatal Vulkan error");
    }
}

b8 vulkan_ui_backend_initialize(Vulkan_Context* context, void* window) {

    CORE_INFO("Initializing ImGui UI backend...");

    // Create ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    if (!ImGui_ImplSDL3_InitForVulkan((SDL_Window*)window)) {
        CORE_ERROR("Failed to initialize ImGui SDL3 backend");
        return false;
    }

    // Setup Vulkan backend init info
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = context->instance;
    init_info.PhysicalDevice = context->device.physical_device;
    init_info.Device = context->device.logical_device;
    init_info.QueueFamily = context->device.graphics_queue_index;
    init_info.Queue = context->device.graphics_queue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPoolSize =
        VULKAN_IMGUI_SHADER_MAX_TEXTURE_COUNT; // Let ImGui create its own
                                               // descriptor pool
    init_info.RenderPass =
        context->ui_renderpass.handle; // Set render pass here
    init_info.Subpass = 0;
    init_info.MinImageCount = context->swapchain.image_count;
    init_info.ImageCount = context->swapchain.image_count;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = context->allocator;
    init_info.CheckVkResultFn = check_vk_result;

    if (!ImGui_ImplVulkan_Init(&init_info)) {
        CORE_ERROR("Failed to initialize ImGui Vulkan backend");
        ImGui_ImplSDL3_Shutdown();
        return false;
    }

    // Fonts are now automatically uploaded during Init

    CORE_INFO("ImGui UI backend initialized successfully");
    return true;
}

void vulkan_ui_backend_shutdown(Vulkan_Context* context) {
    CORE_INFO("Shutting down ImGui UI backend...");

    vkDeviceWaitIdle(context->device.logical_device);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    CORE_INFO("ImGui UI backend shutdown complete");
}

void vulkan_ui_backend_new_frame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void vulkan_ui_backend_render(Vulkan_Context* context,
    VkCommandBuffer command_buffer) {

    // Finish building ImGui draw data
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();

    // Render ImGui draw data
    ImGui_ImplVulkan_RenderDrawData(draw_data, command_buffer);
}
