#include "app_viewport_layer.hpp"
#include "imgui.h"
#include "ui/client_ui.hpp"

// Interfaces from core library
#include <core/logger.hpp>
#include <entry.hpp>
#include <events/events.hpp>
#include <input/input_codes.hpp>
#include <memory/memory.hpp>
#include <ui/ui.hpp>
#include <ui/ui_types.hpp>

#if defined(PLATFORM_WINDOWS) && !defined(VOLTRUM_STATIC_LINKING)
#    include <imgui.h>
#endif

// Client-specific state structure
struct Frontend_State {
    // Add any client-specific state here later
    b8 initialized;
};

// Memory debug callback function
b8 client_memory_debug_callback(const Event* event) {
    if (event->key.key_code == Key_Code::M && !event->key.repeat) {
        u64 allocation_count = memory_get_allocations_count();
        CLIENT_INFO("Current memory allocations: %llu", allocation_count);
    }
    return false; // Don't consume, let other callbacks process
}

// Client lifecycle callback implementations
b8 client_initialize(Client* client_state) {

#if defined(PLATFORM_WINDOWS) && !defined(VOLTRUM_STATIC_LINKING)
    // Get ImGui context from core DLL for Windows compatibility
    // Only needed when using dynamic linking (DLL)
    void* imgui_context = ui_get_imgui_context();
    if (imgui_context) {
        ImGui::SetCurrentContext((ImGuiContext*)imgui_context);
        CLIENT_DEBUG("Set ImGui context from core DLL");
    } else {
        CLIENT_WARN("Failed to get ImGui context from core DLL");
    }
#endif

    // Register memory debug event listener - press 'M' to show allocation count
    events_register_callback(Event_Type::KEY_PRESSED,
        client_memory_debug_callback,
        Event_Priority::LOW);

    // Initialize viewport layer
    if (!app_viewport_layer_initialize()) {
        CLIENT_ERROR("Failed to initialize viewport layer");
        return false;
    }

    CLIENT_INFO("Client initialized.");

    return true;
}

b8 client_update(Client* client_state, f32 delta_time) {
    // Client update logic can go here
    return true;
}

b8 client_render(Client* client_state, f32 delta_time) {
    // Client render logic can go here
    return true;
}

void client_on_resize(Client* client_state, u32 width, u32 height) {
    // Handle resize events
}

void client_shutdown(Client* client_state) {
    // Shutdown viewport layer
    app_viewport_layer_shutdown();

    // Clean up frontend state
    memory_deallocate(client_state->state,
        sizeof(Frontend_State),
        Memory_Tag::CLIENT);

    CLIENT_INFO("Client shutdown complete.")
}

// Main client initialization function called by core
b8 create_client(Client* client_state) {
    // Set up client configuration
    client_state->config.name = "Voltrum EDA";
    client_state->config.width = 1280;
    client_state->config.height = 720;
    client_state->config.theme = UI_Theme::CATPPUCCIN_MOCHA;

    // Set up lifecycle callbacks
    client_state->initialize = client_initialize;
    client_state->update = client_update;
    client_state->render = client_render;
    client_state->on_resize = client_on_resize;
    client_state->shutdown = client_shutdown;

    // Initialize state pointers
    client_state->state =
        memory_allocate(sizeof(Frontend_State), Memory_Tag::CLIENT);

    // Create layers using C++17 compatible initialization
    UI_Layer voltrum_layer;
    voltrum_layer.name = "voltrum_window";
    voltrum_layer.on_render = client_ui_render_voltrum_window;
    voltrum_layer.on_attach = nullptr;
    voltrum_layer.on_detach = nullptr;
    voltrum_layer.component_state = nullptr;
    client_state->layers.push_back(voltrum_layer);

    UI_Layer viewport_layer;
    viewport_layer.name = "viewport_layer";
    viewport_layer.on_render = app_viewport_layer_render;
    viewport_layer.on_attach = nullptr;
    viewport_layer.on_detach = nullptr;
    viewport_layer.component_state = nullptr;
    client_state->layers.push_back(viewport_layer);

    client_state->menu_callback = client_ui_render_menus;

    return true;
}
