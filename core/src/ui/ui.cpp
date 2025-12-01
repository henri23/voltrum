#include "ui.hpp"
#include "ui_themes.hpp"
#include "ui_titlebar.hpp"
#include "ui_viewport.hpp"

#include "core/asserts.hpp"
#include "core/logger.hpp"
#include "events/events.hpp"
#include "input/input_codes.hpp"
#include "memory/memory.hpp"
#include "platform/platform.hpp"
#include "systems/resource_system.hpp"
#include "systems/texture_system.hpp"

#include "renderer/vulkan/vulkan_types.hpp"
#include "renderer/vulkan/vulkan_ui.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

struct UI_State {
    Auto_Array<UI_Layer>* layers;
    UI_Theme current_theme;
    PFN_menu_callback menu_callback;
    const char* app_name;
    b8 is_initialized;

    ImGui_ImplVulkan_InitInfo vulkan_init_info;

    unsigned int dockspace_id;
    b8 dockspace_open;
    b8 window_began; // Track if ImGui::Begin() was called this frame

    ImFont* fonts[(u8)Font_Style::MAX_COUNT];

    Texture* app_icon;
    Texture* minimize_icon;
    Texture* maximize_icon;
    Texture* restore_icon;
    Texture* close_icon;
    b8 is_titlebar_hovered;
    b8 is_menu_hovered;

    b8 is_resizing;
    u32 resize_direction; // Bitmask: LEFT=1, RIGHT=2, TOP=4, BOTTOM=8
    ImVec2 initial_mouse_pos;
    s32 initial_window_x, initial_window_y;
    s32 initial_window_width, initial_window_height;
};

internal_var UI_State state;

UI_State* ui_get_state() { return &state; }

// Forward declarations for internal functions
INTERNAL_FUNC b8 setup_imgui_context(f32 main_scale, void* window);
INTERNAL_FUNC b8 ui_event_handler(const Event* event);
INTERNAL_FUNC ImGuiKey engine_key_to_imgui_key(Key_Code key);
INTERNAL_FUNC int engine_mouse_to_imgui_button(Mouse_Button button);
INTERNAL_FUNC void ui_dockspace_render();
INTERNAL_FUNC b8 load_default_fonts();

b8 ui_initialize(UI_Theme theme,
    Auto_Array<UI_Layer>* layers,
    PFN_menu_callback menu_callback,
    const char* app_name,
    void* window) {

    CORE_DEBUG("Initializing UI subsystem...");

    if (state.is_initialized) {
        CORE_WARN("UI subsystem already initialized");
        return true;
    }

    // Preserve vulkan_init_info before zeroing state
    ImGui_ImplVulkan_InitInfo preserved_vulkan_init_info =
        state.vulkan_init_info;

    // Zero out the state (though it should already be zero)
    memory_zero(&state, sizeof(UI_State));

    // Restore vulkan_init_info
    state.vulkan_init_info = preserved_vulkan_init_info;

    // Set configuration from parameters BEFORE setting up ImGui context
    state.current_theme = theme;

    // Get window details from platform
    u32 width, height;
    f32 main_scale;

    if (!platform_get_window_details(&width, &height, &main_scale)) {
        CORE_ERROR("Failed to get window details for UI initialization");
        return false;
    }

    // Setup ImGui context (now with correct theme in ui_state)
    if (!setup_imgui_context(main_scale, window)) {
        CORE_ERROR("Failed to setup ImGui context");
        return false;
    }

    // Set remaining configuration parameters
    state.layers = layers;

    state.is_initialized = true;

    // Initialize dockspace state
    state.dockspace_id = 0; // Will be set on first render
    state.dockspace_open = true;
    state.window_began = false;

    // Initialize font subsystem
    if (!load_default_fonts()) {
        CORE_ERROR("Failed to initialize font system");
        return false;
    }

    // Setup ImGui docking
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    CORE_DEBUG("ImGui docking enabled");

    // Setup infrastructure components (titlebar icons loaded later after Vulkan
    // init)
    state.menu_callback = menu_callback;
    state.app_name = app_name;

    // Register UI event callbacks with LOWEST priority so canvas can override
    events_register_callback(Event_Type::KEY_PRESSED,
        ui_get_event_callback(),
        Event_Priority::LOWEST);
    events_register_callback(Event_Type::KEY_RELEASED,
        ui_get_event_callback(),
        Event_Priority::LOWEST);
    events_register_callback(Event_Type::MOUSE_BUTTON_PRESSED,
        ui_get_event_callback(),
        Event_Priority::LOWEST);
    events_register_callback(Event_Type::MOUSE_BUTTON_RELEASED,
        ui_get_event_callback(),
        Event_Priority::LOWEST);
    events_register_callback(Event_Type::MOUSE_MOVED,
        ui_get_event_callback(),
        Event_Priority::LOWEST);
    events_register_callback(Event_Type::MOUSE_WHEEL_SCROLLED,
        ui_get_event_callback(),
        Event_Priority::LOWEST);

    CORE_INFO("UI subsystem initialized successfully");
    return true;
}

void ui_load_resources() {
    // Load titlebar icons now that texture system is initialized
    extern Vulkan_Context* vulkan_get_context();
    ui_titlebar_setup(state.menu_callback,
        state.app_name,
        vulkan_get_context());
}

void ui_shutdown() {
    CORE_DEBUG("Shutting down UI subsystem...");

    if (!state.is_initialized) {
        CORE_WARN("UI subsystem not initialized");
        return;
    }

    // Unregister UI event callbacks
    events_unregister_callback(Event_Type::KEY_PRESSED,
        ui_get_event_callback());
    events_unregister_callback(Event_Type::KEY_RELEASED,
        ui_get_event_callback());
    events_unregister_callback(Event_Type::MOUSE_BUTTON_PRESSED,
        ui_get_event_callback());
    events_unregister_callback(Event_Type::MOUSE_BUTTON_RELEASED,
        ui_get_event_callback());
    events_unregister_callback(Event_Type::MOUSE_MOVED,
        ui_get_event_callback());
    events_unregister_callback(Event_Type::MOUSE_WHEEL_SCROLLED,
        ui_get_event_callback());

    // Note: All ImGui cleanup happens in ui_cleanup_vulkan_resources after
    // Vulkan synchronization
    CORE_DEBUG(
        "ImGui backends and context will be destroyed by renderer during "
        "Vulkan cleanup");

    // Cleanup component system
    if (!state.layers->empty()) {
        // Call detach callbacks for all active components
        for (u32 i = 0; i < state.layers->size(); ++i) {
            UI_Layer* component = &state.layers->data[i];
            if (component->on_detach) {

                component->on_detach(component->component_state);
                CORE_DEBUG("Component %u detach callback complete", i);
            }
        }

        state.layers->clear();
        CORE_DEBUG("UI components cleared.");
    }

    // Reset state
    state.is_initialized = false;
    state.current_theme = UI_Theme::DARK;
    state.menu_callback = nullptr;

    // Clear any remaining ImGui state pointers to prevent access after shutdown
    // Note: Actual ImGui context destruction happens in
    // ui_cleanup_vulkan_resources

    CORE_DEBUG("UI subsystem shut down successfully");
}

ImFont* ui_get_font(Font_Style style) { return state.fonts[(u8)style]; }

// Internal UI event handler for engine events - translate to ImGui native API
INTERNAL_FUNC b8 ui_event_handler(const Event* event) {
    if (!state.is_initialized) {
        return false;
    }

    // Don't consume ESC key - let platform handle it for quit functionality
    if (event->type == Event_Type::KEY_PRESSED &&
        event->key.key_code == Key_Code::ESCAPE) {
        return false;
    }

    // Get ImGui IO for direct input
    ImGuiIO& io = ImGui::GetIO();

    // Translate engine events to ImGui native input API
    switch (event->type) {
    case Event_Type::KEY_PRESSED:
    case Event_Type::KEY_RELEASED: {
        ImGuiKey imgui_key = engine_key_to_imgui_key(event->key.key_code);
        if (imgui_key != ImGuiKey_None) {
            io.AddKeyEvent(imgui_key, event->type == Event_Type::KEY_PRESSED);
        }
        return io.WantCaptureKeyboard; // Consume if ImGui wants keyboard input
    }

    case Event_Type::MOUSE_BUTTON_PRESSED:
    case Event_Type::MOUSE_BUTTON_RELEASED: {
        int imgui_button =
            engine_mouse_to_imgui_button(event->mouse_button.button);
        if (imgui_button >= 0) {
            io.AddMouseButtonEvent(imgui_button,
                event->type == Event_Type::MOUSE_BUTTON_PRESSED);
        }

        return io.WantCaptureMouse; // Consume if ImGui wants mouse input
    }

    case Event_Type::MOUSE_MOVED:
        io.AddMousePosEvent((float)event->mouse_move.x,
            (float)event->mouse_move.y);
        return io.WantCaptureMouse; // Consume if ImGui wants mouse input

    case Event_Type::MOUSE_WHEEL_SCROLLED:
        io.AddMouseWheelEvent(event->mouse_wheel.delta_x,
            event->mouse_wheel.delta_y);
        return io.WantCaptureMouse; // Consume if ImGui wants mouse input

    default:
        return false;
    }
}

// Internal function implementations
INTERNAL_FUNC b8 setup_imgui_context(f32 main_scale, void* window) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    // Configure ImGui
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking

// SDL3 viewport support is experimental and can cause crashes
// Only enable if explicitly requested and working properly
#ifdef VOLTRUM_ENABLE_VIEWPORTS
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport
                                                        // / Platform Windows
    CORE_DEBUG("ImGui viewports enabled (experimental with SDL3)");
#else
    CORE_DEBUG("ImGui viewports disabled (SDL3 compatibility mode)");
#endif

    // Setup style and theme using the theme system
    ImGuiStyle& style = ImGui::GetStyle();

    // Apply the configured theme
    ui_themes_apply(state.current_theme, style);

#ifndef PLATFORM_APPLE
    // Setup scaling
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;
#endif

// When viewports are enabled we tweak WindowRounding/WindowBg so platform
// windows can look identical to regular ones.
#ifdef VOLTRUM_ENABLE_VIEWPORTS
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
#endif

    // Initialize platform backend for ImGui
    if (!window) {
        CORE_ERROR("SDL window not available for UI initialization");
        return false;
    }

    // Initialize SDL3 backend for ImGui
    if (!ImGui_ImplSDL3_InitForVulkan((SDL_Window*)window)) {
        CORE_ERROR("Failed to initialize ImGui SDL3 backend");
        return false;
    }

    // Initialize ImGui Vulkan backend using prepared init info
    if (!ImGui_ImplVulkan_Init(&state.vulkan_init_info)) {
        CORE_ERROR("Failed to initialize ImGui Vulkan backend");
        return false;
    }

    CORE_DEBUG("ImGui context setup completed");
    return true;
}

// Component system implementation

// Internal accessor for current theme (for core UI components only)
UI_Theme ui_get_current_theme() { return state.current_theme; }

// Set UI theme at runtime
void ui_change_theme(UI_Theme theme) {
    if ((int)theme >= (int)UI_Theme::COUNT) {
        CORE_WARN("Invalid theme index, defaulting to DARK");
        theme = UI_Theme::DARK;
    }

    state.current_theme = theme;

    // Re-apply theme to ImGui style
    ImGuiStyle& style = ImGui::GetStyle();
    ui_themes_apply(theme, style);

    CORE_DEBUG("Theme changed to: %d", (int)theme);
}

// Expose UI event callback for application registration
PFN_event_callback ui_get_event_callback() { return ui_event_handler; }

// Get ImGui context for Windows DLL compatibility
void* ui_get_imgui_context() { return ImGui::GetCurrentContext(); }

// ========================================================================
// UI → Renderer Interface Implementation
// ========================================================================

b8 ui_setup_vulkan_resources(Vulkan_Context* context) {
    CORE_DEBUG("Setting up UI Vulkan resources...");

    // Create descriptor set layout for ImGui textures
    VkDescriptorSetLayoutBinding binding = {};
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    binding.binding = 0;

    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 1;
    layout_info.pBindings = &binding;

    VK_CHECK(vkCreateDescriptorSetLayout(context->device.logical_device,
        &layout_info,
        context->allocator,
        &context->ui_descriptor_set_layout));

    // Create descriptor pool for ImGui textures
    VkDescriptorPoolSize pool_size = {};
    pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_size.descriptorCount = 1000; // Support up to 1000 textures

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;

    VK_CHECK(vkCreateDescriptorPool(context->device.logical_device,
        &pool_info,
        context->allocator,
        &context->ui_descriptor_pool));

    // Create linear sampler for ImGui textures
    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.minLod = -1000;
    sampler_info.maxLod = 1000;
    sampler_info.maxAnisotropy = 1.0f;

    VK_CHECK(vkCreateSampler(context->device.logical_device,
        &sampler_info,
        context->allocator,
        &context->ui_linear_sampler));

    // Prepare ImGui Vulkan backend initialization info for later use
    state.vulkan_init_info = {};
    state.vulkan_init_info.Instance = context->instance;
    state.vulkan_init_info.PhysicalDevice = context->device.physical_device;
    state.vulkan_init_info.Device = context->device.logical_device;
    state.vulkan_init_info.QueueFamily = context->device.graphics_queue_index;
    state.vulkan_init_info.Queue = context->device.graphics_queue;
    state.vulkan_init_info.DescriptorPool = context->ui_descriptor_pool;
    state.vulkan_init_info.DescriptorPoolSize = 0;
    state.vulkan_init_info.MinImageCount =
        context->swapchain.max_in_flight_frames;
    state.vulkan_init_info.ImageCount = context->swapchain.image_count;
    state.vulkan_init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    state.vulkan_init_info.RenderPass = context->ui_renderpass.handle;
    state.vulkan_init_info.Subpass = 0;
    state.vulkan_init_info.Allocator = context->allocator;

    CORE_DEBUG("UI Vulkan resources created successfully");
    return true;
}

void ui_cleanup_vulkan_resources(Vulkan_Context* context) {
    CORE_DEBUG("Cleaning up UI Vulkan resources...");

    // Clean up all UI component Vulkan resources first
    ui_viewport_cleanup_vulkan_resources(context);

    // Shutdown ImGui backends AFTER component cleanup and Vulkan
    // synchronization
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    CORE_DEBUG("ImGui backends shutdown complete after Vulkan synchronization");

    // Clear backend user data manually to prevent assertion
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererUserData = nullptr;
    io.BackendPlatformUserData = nullptr;

    // Destroy ImGui context after backend shutdown
    ImGui::DestroyContext();
    CORE_DEBUG("ImGui context destroyed after Vulkan cleanup");

    if (context->ui_linear_sampler) {
        vkDestroySampler(context->device.logical_device,
            context->ui_linear_sampler,
            context->allocator);
        context->ui_linear_sampler = VK_NULL_HANDLE;
    }

    if (context->ui_descriptor_pool) {
        vkDestroyDescriptorPool(context->device.logical_device,
            context->ui_descriptor_pool,
            context->allocator);
        context->ui_descriptor_pool = VK_NULL_HANDLE;
    }

    if (context->ui_descriptor_set_layout) {
        vkDestroyDescriptorSetLayout(context->device.logical_device,
            context->ui_descriptor_set_layout,
            context->allocator);
        context->ui_descriptor_set_layout = VK_NULL_HANDLE;
    }

    CORE_DEBUG("UI Vulkan resources cleanup completed");
}

void ui_on_vulkan_resize(Vulkan_Context* context, u32 width, u32 height) {
    // UI doesn't need special resize handling for now
    // ImGui automatically adapts to new framebuffer size
}

void ui_begin_vulkan_frame() {
    if (!state.is_initialized) {
        return;
    }

    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

b8 ui_draw_components(Vulkan_Command_Buffer* command_buffer) {
    if (!state.is_initialized) {
        CORE_ERROR(
            "ui_render_vulkan_components called, but ui state is not "
            "intialized");
        return false;
    }

    // Render all UI components using the component system
    // Render dockspace first - it provides the container for other windows
    ui_dockspace_render();

    // Render custom titlebar if enabled
    ui_titlebar_draw();

    // Render all registered UI components
    for (u32 i = 0; i < state.layers->length; ++i) {
        UI_Layer* layer = &state.layers->data[i];

        if (layer->on_render)
            layer->on_render(layer->component_state);
    }

    // Finalize rendering and get draw data
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();

// Update and Render additional Platform Windows (for
// ImGuiConfigFlags_ViewportsEnable) Only render viewports if they're enabled
// and working properly
#ifdef VOLTRUM_ENABLE_VIEWPORTS
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
#endif

    // Check if minimized
    const bool is_minimized =
        (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

    if (is_minimized) {
        CORE_WARN(
            "ui_render_vulkan_components called, but the window is minimized");
        return false;
    }

    // Record ImGui draw commands to the current command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, command_buffer->handle);

    return true;
}

// Convert engine key codes to ImGui key codes
INTERNAL_FUNC ImGuiKey engine_key_to_imgui_key(Key_Code key) {
    switch (key) {
    case Key_Code::SPACE:
        return ImGuiKey_Space;
    case Key_Code::APOSTROPHE:
        return ImGuiKey_Apostrophe;
    case Key_Code::COMMA:
        return ImGuiKey_Comma;
    case Key_Code::MINUS:
        return ImGuiKey_Minus;
    case Key_Code::PERIOD:
        return ImGuiKey_Period;
    case Key_Code::SLASH:
        return ImGuiKey_Slash;
    case Key_Code::KEY_0:
        return ImGuiKey_0;
    case Key_Code::KEY_1:
        return ImGuiKey_1;
    case Key_Code::KEY_2:
        return ImGuiKey_2;
    case Key_Code::KEY_3:
        return ImGuiKey_3;
    case Key_Code::KEY_4:
        return ImGuiKey_4;
    case Key_Code::KEY_5:
        return ImGuiKey_5;
    case Key_Code::KEY_6:
        return ImGuiKey_6;
    case Key_Code::KEY_7:
        return ImGuiKey_7;
    case Key_Code::KEY_8:
        return ImGuiKey_8;
    case Key_Code::KEY_9:
        return ImGuiKey_9;
    case Key_Code::SEMICOLON:
        return ImGuiKey_Semicolon;
    case Key_Code::EQUALS:
        return ImGuiKey_Equal;
    case Key_Code::A:
        return ImGuiKey_A;
    case Key_Code::B:
        return ImGuiKey_B;
    case Key_Code::C:
        return ImGuiKey_C;
    case Key_Code::D:
        return ImGuiKey_D;
    case Key_Code::E:
        return ImGuiKey_E;
    case Key_Code::F:
        return ImGuiKey_F;
    case Key_Code::G:
        return ImGuiKey_G;
    case Key_Code::H:
        return ImGuiKey_H;
    case Key_Code::I:
        return ImGuiKey_I;
    case Key_Code::J:
        return ImGuiKey_J;
    case Key_Code::K:
        return ImGuiKey_K;
    case Key_Code::L:
        return ImGuiKey_L;
    case Key_Code::M:
        return ImGuiKey_M;
    case Key_Code::N:
        return ImGuiKey_N;
    case Key_Code::O:
        return ImGuiKey_O;
    case Key_Code::P:
        return ImGuiKey_P;
    case Key_Code::Q:
        return ImGuiKey_Q;
    case Key_Code::R:
        return ImGuiKey_R;
    case Key_Code::S:
        return ImGuiKey_S;
    case Key_Code::T:
        return ImGuiKey_T;
    case Key_Code::U:
        return ImGuiKey_U;
    case Key_Code::V:
        return ImGuiKey_V;
    case Key_Code::W:
        return ImGuiKey_W;
    case Key_Code::X:
        return ImGuiKey_X;
    case Key_Code::Y:
        return ImGuiKey_Y;
    case Key_Code::Z:
        return ImGuiKey_Z;
    case Key_Code::LEFTBRACKET:
        return ImGuiKey_LeftBracket;
    case Key_Code::BACKSLASH:
        return ImGuiKey_Backslash;
    case Key_Code::RIGHTBRACKET:
        return ImGuiKey_RightBracket;
    case Key_Code::GRAVE:
        return ImGuiKey_GraveAccent;
    case Key_Code::ESCAPE:
        return ImGuiKey_Escape;
    case Key_Code::RETURN:
        return ImGuiKey_Enter;
    case Key_Code::TAB:
        return ImGuiKey_Tab;
    case Key_Code::BACKSPACE:
        return ImGuiKey_Backspace;
    case Key_Code::INSERT:
        return ImGuiKey_Insert;
    case Key_Code::DELETE:
        return ImGuiKey_Delete;
    case Key_Code::RIGHT:
        return ImGuiKey_RightArrow;
    case Key_Code::LEFT:
        return ImGuiKey_LeftArrow;
    case Key_Code::DOWN:
        return ImGuiKey_DownArrow;
    case Key_Code::UP:
        return ImGuiKey_UpArrow;
    case Key_Code::PAGEUP:
        return ImGuiKey_PageUp;
    case Key_Code::PAGEDOWN:
        return ImGuiKey_PageDown;
    case Key_Code::HOME:
        return ImGuiKey_Home;
    case Key_Code::END:
        return ImGuiKey_End;
    case Key_Code::CAPSLOCK:
        return ImGuiKey_CapsLock;
    case Key_Code::SCROLLLOCK:
        return ImGuiKey_ScrollLock;
    case Key_Code::PRINTSCREEN:
        return ImGuiKey_PrintScreen;
    case Key_Code::PAUSE:
        return ImGuiKey_Pause;
    case Key_Code::F1:
        return ImGuiKey_F1;
    case Key_Code::F2:
        return ImGuiKey_F2;
    case Key_Code::F3:
        return ImGuiKey_F3;
    case Key_Code::F4:
        return ImGuiKey_F4;
    case Key_Code::F5:
        return ImGuiKey_F5;
    case Key_Code::F6:
        return ImGuiKey_F6;
    case Key_Code::F7:
        return ImGuiKey_F7;
    case Key_Code::F8:
        return ImGuiKey_F8;
    case Key_Code::F9:
        return ImGuiKey_F9;
    case Key_Code::F10:
        return ImGuiKey_F10;
    case Key_Code::F11:
        return ImGuiKey_F11;
    case Key_Code::F12:
        return ImGuiKey_F12;
    case Key_Code::KP_0:
        return ImGuiKey_Keypad0;
    case Key_Code::KP_1:
        return ImGuiKey_Keypad1;
    case Key_Code::KP_2:
        return ImGuiKey_Keypad2;
    case Key_Code::KP_3:
        return ImGuiKey_Keypad3;
    case Key_Code::KP_4:
        return ImGuiKey_Keypad4;
    case Key_Code::KP_5:
        return ImGuiKey_Keypad5;
    case Key_Code::KP_6:
        return ImGuiKey_Keypad6;
    case Key_Code::KP_7:
        return ImGuiKey_Keypad7;
    case Key_Code::KP_8:
        return ImGuiKey_Keypad8;
    case Key_Code::KP_9:
        return ImGuiKey_Keypad9;
    case Key_Code::KP_PERIOD:
        return ImGuiKey_KeypadDecimal;
    case Key_Code::KP_DIVIDE:
        return ImGuiKey_KeypadDivide;
    case Key_Code::KP_MULTIPLY:
        return ImGuiKey_KeypadMultiply;
    case Key_Code::KP_MINUS:
        return ImGuiKey_KeypadSubtract;
    case Key_Code::KP_PLUS:
        return ImGuiKey_KeypadAdd;
    case Key_Code::KP_ENTER:
        return ImGuiKey_KeypadEnter;
    case Key_Code::LSHIFT:
        return ImGuiKey_LeftShift;
    case Key_Code::LCTRL:
        return ImGuiKey_LeftCtrl;
    case Key_Code::LALT:
        return ImGuiKey_LeftAlt;
    case Key_Code::LGUI:
        return ImGuiKey_LeftSuper;
    case Key_Code::RSHIFT:
        return ImGuiKey_RightShift;
    case Key_Code::RCTRL:
        return ImGuiKey_RightCtrl;
    case Key_Code::RALT:
        return ImGuiKey_RightAlt;
    case Key_Code::RGUI:
        return ImGuiKey_RightSuper;
    default:
        return ImGuiKey_None;
    }
}

// Convert engine mouse buttons to ImGui mouse buttons
INTERNAL_FUNC int engine_mouse_to_imgui_button(Mouse_Button button) {
    switch (button) {
    case Mouse_Button::LEFT:
        return 0;
    case Mouse_Button::RIGHT:
        return 1;
    case Mouse_Button::MIDDLE:
        return 2;
    case Mouse_Button::X1:
        return 3;
    case Mouse_Button::X2:
        return 4;
    default:
        return -1;
    }
}

// Internal dockspace rendering function
INTERNAL_FUNC void ui_dockspace_render() {
    // Reset the window_began flag at the start of each frame
    state.window_began = false;

    // Generate dockspace ID on first use (when ImGui context is ready)
    if (state.dockspace_id == 0) {
        state.dockspace_id = ImGui::GetID("MainDockspace");
        CORE_DEBUG("Generated dockspace ID: %u", state.dockspace_id);
    }

    // Setup viewport for fullscreen dockspace
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos;
    ImVec2 work_size = viewport->WorkSize;

    // Adjust for custom titlebar if enabled
    f32 titlebar_height = TITLEBAR_HEIGHT;
    work_pos.y += titlebar_height;
    work_size.y -= titlebar_height;

    // Set up the main dockspace window
    ImGui::SetNextWindowPos(work_pos);
    ImGui::SetNextWindowSize(work_size);
    ImGui::SetNextWindowViewport(viewport->ID);

    // Configure window for fullscreen dockspace
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    // Setup window flags for dockspace
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

    // Begin the main dockspace window
    const char* window_name = "DockSpace";
    ImGui::Begin(window_name, &state.dockspace_open, window_flags);
    state.window_began = true;

    // Pop style vars
    ImGui::PopStyleVar(3);

    // Create the dockspace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        // Set minimum window size for better docking experience
        ImGuiStyle& style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 300.0f;

        // Create dockspace
        ImGui::DockSpace(state.dockspace_id);

        // Restore original window size
        style.WindowMinSize.x = minWinSizeX;
    } else {
        CORE_ERROR("ImGui docking is not enabled!");
    }

    // End the dockspace window
    if (state.window_began) {
        ImGui::End(); // End DockSpace window
        state.window_began = false;
    }
}

b8 load_default_fonts() {
    ImGuiIO& io = ImGui::GetIO();

    constexpr const char* style_names[] = {"normal",
        "italic",
        "bold_normal",
        "bold_italic"};

    // Loop through all combinations and load fonts
    for (u8 s = 0; s < (u8)Font_Style::MAX_COUNT; ++s) {
        // Build resource path: "jetbrains/jetbrains_{style}"
        char path[128];
        u32 i = 0;

        const char* prefix = "jetbrains/jetbrains_";
        for (u32 j = 0; prefix[j] != '\0'; ++j)
            path[i++] = prefix[j];

        for (u32 j = 0; style_names[s][j] != '\0'; ++j)
            path[i++] = style_names[s][j];

        path[i] = '\0';

        // Load from resource system
        Resource resource = {};
        if (!resource_system_load(path, Resource_Type::FONT, &resource)) {
            CORE_ERROR("Failed to load font: %s", path);
            state.fonts[s] = nullptr;
            continue;
        }

        // Add to ImGui
        ImFontConfig config = {};
        config.FontDataOwnedByAtlas = false;

        state.fonts[s] = io.Fonts->AddFontFromMemoryTTF(resource.data,
            resource.data_size,
            20.0f,
            &config);

        // Unload resource immediately - ImGui has copied the data
        resource_system_unload(&resource);

        if (state.fonts[s]) {
            CORE_DEBUG("Loaded font: %s at %.0fpt", path, 20.0f);
        }
    }

    // Build atlas
    if (!io.Fonts->Build()) {
        CORE_ERROR("Failed to build font atlas");
        return false;
    }

    // Set default font
    io.FontDefault = state.fonts[(u8)Font_Style::NORMAL];

    return true;
}

// ============================================================================
// NEW RENDER CONTEXT API (CLEAN SEPARATION)
// ============================================================================

UI_Render_Context ui_create_render_context(const UI_Renderer_Info* info) {
    if (!state.is_initialized) {
        CORE_ERROR("UI not initialized - call ui_initialize() first");
        return nullptr;
    }

    CORE_DEBUG("Creating UI render context with Vulkan resources...");

    // Store Vulkan device info (will replace vulkan_init_info in final version)
    // For now, create the Vulkan resources directly

    // 1. Create descriptor pool
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000}};
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = pool_sizes;

    VkDescriptorPool descriptor_pool;
    if (vkCreateDescriptorPool(info->device,
            &pool_info,
            info->allocator,
            &descriptor_pool) != VK_SUCCESS) {
        CORE_ERROR("Failed to create UI descriptor pool");
        return nullptr;
    }

    // 2. Initialize ImGui Vulkan backend
    state.vulkan_init_info.Instance = info->instance;
    state.vulkan_init_info.PhysicalDevice = info->physical_device;
    state.vulkan_init_info.Device = info->device;
    state.vulkan_init_info.QueueFamily = info->graphics_queue_family;
    state.vulkan_init_info.Queue = info->graphics_queue;
    state.vulkan_init_info.DescriptorPool = descriptor_pool;
    state.vulkan_init_info.RenderPass = info->ui_renderpass;
    state.vulkan_init_info.MinImageCount = info->min_image_count;
    state.vulkan_init_info.ImageCount = info->image_count;
    state.vulkan_init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    state.vulkan_init_info.Allocator = info->allocator;

    ImGui_ImplVulkan_Init(&state.vulkan_init_info);

    // 3. Load titlebar icon textures (merged from ui_load_resources)
    state.app_icon = texture_system_acquire("voltrum_icon", true);
    state.minimize_icon = texture_system_acquire("window_minimize", true);
    state.maximize_icon = texture_system_acquire("window_maximize", true);
    state.restore_icon = texture_system_acquire("window_restore", true);
    state.close_icon = texture_system_acquire("window_close", true);

    // 4. Create descriptor sets for icons
    Texture* icons[] = {state.app_icon,
        state.minimize_icon,
        state.maximize_icon,
        state.restore_icon,
        state.close_icon};
    for (u32 i = 0; i < 5; ++i) {
        if (!icons[i]) {
            CORE_WARN("Failed to load titlebar icon %u", i);
            continue;
        }

        Vulkan_Texture_Data* data =
            (Vulkan_Texture_Data*)icons[i]->internal_data;

        if (!data) {
            CORE_ERROR("Texture has null internal data");
            continue;
        }

        data->descriptor_set = ImGui_ImplVulkan_AddTexture(data->sampler,
            data->image.view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    CORE_INFO("UI render context created successfully");
    return (UI_Render_Context)&state;
}

void ui_destroy_render_context(UI_Render_Context ctx) {
    if (!ctx)
        return;

    CORE_DEBUG("Destroying UI render context...");

    // Cleanup titlebar descriptor sets
    Texture* icons[] = {state.app_icon,
        state.minimize_icon,
        state.maximize_icon,
        state.restore_icon,
        state.close_icon};
    for (u32 i = 0; i < 5; ++i) {
        if (icons[i]) {
            Vulkan_Texture_Data* data =
                (Vulkan_Texture_Data*)icons[i]->internal_data;

            if (data && data->descriptor_set != VK_NULL_HANDLE) {
                ImGui_ImplVulkan_RemoveTexture(data->descriptor_set);
                data->descriptor_set = VK_NULL_HANDLE;
            }
            texture_system_release(icons[i]->name);
        }
    }

    // Shutdown ImGui backends
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    CORE_DEBUG("UI render context destroyed");
}

void ui_begin_frame(UI_Render_Context ctx) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void ui_render_frame(UI_Render_Context ctx, VkCommandBuffer cmd_buffer) {
    // Render dockspace
    ui_dockspace_render();

    // Render custom titlebar
    ui_titlebar_draw();

    // Render all registered UI components
    for (u32 i = 0; i < state.layers->length; ++i) {
        UI_Layer* layer = &state.layers->data[i];
        if (layer->on_render)
            layer->on_render(layer->component_state);
    }

    // Finalize rendering
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();

    if (draw_data->DisplaySize.x > 0 && draw_data->DisplaySize.y > 0) {
        ImGui_ImplVulkan_RenderDrawData(draw_data, cmd_buffer);
    }
}
