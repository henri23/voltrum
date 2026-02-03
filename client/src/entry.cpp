#include "defines.hpp"
#include "editor/editor_layer.hpp"

// Interfaces from core library
#include <core/logger.hpp>
#include <entry.hpp>
#include <events/events.hpp>
#include <input/input.hpp>
#include <input/input_codes.hpp>
#include <memory/memory.hpp>
#include <ui/icons.hpp>
#include <ui/ui_widgets.hpp>

// WARN: This is temporary, the renderer subsystem should not be
// exposed outside the engine
#include <platform/platform.hpp>
#include <renderer/renderer_frontend.hpp>

#if defined(PLATFORM_WINDOWS) && !defined(VOLTRUM_STATIC_LINKING)
#    include <imgui.h>
#endif

// Client-specific state structure
struct Global_Client_State
{
    b8 initialized;
};

// Memory debug callback function
b8
client_memory_debug_callback(const Event *event)
{
    if (event->key.key_code == Key_Code::M && !event->key.repeat)
    {
        u64 allocation_count = memory_get_allocations_count();
        CLIENT_INFO("Current memory allocations: %llu", allocation_count);
    }
    return false; // Don't consume, let other callbacks process
}

// Client lifecycle callback implementations
b8
client_initialize(Client *client_state)
{

#if defined(PLATFORM_WINDOWS) && !defined(VOLTRUM_STATIC_LINKING)
    // Get ImGui context from core DLL for Windows compatibility
    // Only needed when using dynamic linking (DLL)
    // Commented out for UI rewrite
    // void* imgui_context = ui_get_imgui_context();
    // if (imgui_context) {
    //     ImGui::SetCurrentContext((ImGuiContext*)imgui_context);
    //     CLIENT_DEBUG("Set ImGui context from core DLL");
    // } else {
    //     CLIENT_WARN("Failed to get ImGui context from core DLL");
    // }
#endif

    events_register_callback(Event_Type::KEY_PRESSED,
                             client_memory_debug_callback,
                             Event_Priority::LOW);

    CLIENT_INFO("Client initialized.");

    return true;
}

b8
client_update(Client *client_state, Frame_Context *ctx)
{
    static u64 alloc_count      = 0;
    u64        prev_alloc_count = alloc_count;

    alloc_count = memory_get_allocations_count();
    if (input_is_key_pressed(Key_Code::M) && input_was_key_pressed(Key_Code::M))
    {
        CORE_DEBUG("Allocations: %llu (%llu this frame)",
                   alloc_count,
                   alloc_count - prev_alloc_count);
    }

    // TODO: Re-enable when client_update receives Frame_Context*
    if (input_is_key_pressed(Key_Code::T) && input_was_key_pressed(Key_Code::T))
    {
        Event event = {};
        event.type  = Event_Type::DEBUG0;
        ctx->event_queue->enqueue(event);
    }

    return true;
}

b8
client_render(Client *client_state, Frame_Context *ctx)
{
    // Client render logic can go here
    return true;
}

void
client_on_resize(Client *client_state, u32 width, u32 height)
{
    // Handle resize events
}

void
client_shutdown(Client *client_state)
{
    CLIENT_INFO("Client shutdown complete.");
}

// Menu callback - called by core UI to draw menu items
void
client_menu_callback()
{
    if (ui::BeginMenu("File"))
    {
        if (ui::MenuItem(ICON_FA_RIGHT_FROM_BRACKET " Exit"))
        {
            platform_close_window();
        }
        ui::EndMenu();
    }

    if (ui::BeginMenu("View"))
    {
        ui::MenuItem(ICON_FA_WINDOW_MAXIMIZE " Viewport");
        ui::MenuItem(ICON_FA_SLIDERS " Properties");

        ImGui::Separator();

        b8 signal_visible = editor_is_signal_analyzer_visible();
        if (ui::MenuItem(ICON_FA_BOLT " Signal Analyzer",
                         nullptr,
                         !signal_visible))
        {
            editor_toggle_signal_analyzer();
        }

        ImGui::Separator();

        b8 demo_visible = editor_is_demo_window_visible();
        if (ui::MenuItem(ICON_FA_CODE " ImGui Demo", nullptr, !demo_visible))
        {
            editor_toggle_demo_window();
        }

        b8 implot_demo_visible = editor_is_implot_demo_window_visible();
        if (ui::MenuItem(ICON_FA_CHART_LINE " ImPlot Demo",
                         nullptr,
                         !implot_demo_visible))
        {
            editor_toggle_implot_demo_window();
        }
        ui::EndMenu();
    }

    if (ui::BeginMenu("Help"))
    {
        ui::MenuItem(ICON_FA_CIRCLE_INFO " About");
        ui::EndMenu();
    }

    if (ui::BeginMenu("Tools"))
    {
        ui::MenuItem(ICON_FA_GEARS " Explore");
        ui::EndMenu();
    }
}

App_Config
request_client_config()
{
    App_Config client_config;

    // Set up client configuration
    client_config.name   = STR("Voltrum EDA");
    client_config.width  = 1600;
    client_config.height = 900;
    client_config.theme  = UI_Theme::CATPPUCCIN;

    return client_config;
}

// Main client initialization function called by core
b8
create_client(Client *client)
{
    // Set up lifecycle callbacks
    client->initialize = client_initialize;
    client->update     = client_update;
    client->render     = client_render;
    client->on_resize  = client_on_resize;
    client->shutdown   = client_shutdown;

    // Initialize state pointers
    client->state = push_struct(client->mode_arena, Global_Client_State);
    auto editor_layer_state =
        push_struct(client->mode_arena, Editor_Layer_State);

    // Create and register editor layer
    client->layers.init(client->mode_arena);

    // Add layers
    client->layers.add(create_editor_layer(editor_layer_state));

    // Set menu callback
    client->menu_callback = client_menu_callback;

    return true;
}
