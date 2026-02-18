#include "defines.hpp"
#include "editor/editor_layer.hpp"
#include "global_client_state.hpp"
#include "titlebar/titlebar_content.hpp"
#include "utilities/components/command_palette_component.hpp"
#include "utilities/components/settings_component.hpp"
#include "utilities/components/theme_selector_component.hpp"
#include "utilities/utilities_layer.hpp"

#ifdef DEBUG_BUILD
#    include "debug/debug_layer.hpp"
#endif

// Interfaces from core library
#include <core/frame_context.hpp>
#include <core/logger.hpp>
#include <entry.hpp>
#include <events/events.hpp>
#include <input/input.hpp>
#include <input/input_codes.hpp>
#include <memory/memory.hpp>

// WARN: This is temporary, the renderer subsystem should not be
// exposed outside the engine
#include <renderer/renderer_frontend.hpp>
#include <ui/ui.hpp>

#if defined(PLATFORM_WINDOWS) && !defined(VOLTRUM_STATIC_LINKING)
#    include <imgui.h>
#endif

// Client lifecycle callback implementations
b8
client_initialize(Client *client_instance)
{
    Global_Client_State *g_state =
        (Global_Client_State *)client_instance->state;

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

    UI_Theme current_theme          = UI_Theme::DARK;
    ui_get_theme_state(&current_theme, &g_state->theme_palette);
    g_state->target_theme           = current_theme;
    g_state->requested_theme        = current_theme;
    g_state->request_theme_change   = false;
    g_state->is_theme_transitioning = false;
    g_state->theme_transition_t     = 1.0f;

    g_state->theme_transition_from = g_state->theme_palette;
    g_state->theme_transition_to   = g_state->theme_palette;

    CLIENT_INFO("Client initialized.");

    return true;
}

b8
client_update(Client *client, Frame_Context *ctx)
{
    Global_Client_State *g_state = (Global_Client_State *)client->state;
#ifdef DEBUG_BUILD
    {
        local_persist b8 f12_was_down = false;
        b8               f12_is_down  = input_is_key_pressed(Key_Code::F12);
        if (f12_is_down && !f12_was_down)
        {
            // TODO: Delete the layer from the layer stack
            g_state->is_debug_layer_visible = !g_state->is_debug_layer_visible;
        }
        f12_was_down = f12_is_down;
    }
#endif

    // TODO: Re-enable when client_update receives Frame_Context*
    if (input_is_key_pressed(Key_Code::T) && input_was_key_pressed(Key_Code::T))
    {
        Event event = {};
        event.type  = Event_Type::DEBUG0;
        event_queue_produce(ctx->event_queue, event);
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

App_Config
request_client_config()
{
    App_Config client_config;

    // Set up client configuration
    client_config.name   = STR_LIT("Voltrum EDA");
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
    client->state = push_struct(client->project_arena, Global_Client_State);

    auto g_state = (Global_Client_State *)client->state;

    g_state->is_imgui_demo_visible  = true;
    g_state->is_implot_demo_visible = true;
    g_state->target_theme           = UI_Theme::CATPPUCCIN;
    g_state->requested_theme        = UI_Theme::CATPPUCCIN;
    g_state->theme_transition_t     = 1.0f;
    g_state->mode                   = Client_Mode::SCHEMATIC;

    auto editor_layer_state =
        push_struct(client->project_arena, Editor_Layer_State);
    auto utilities_layer_state =
        push_struct(client->project_arena, Utilities_Layer_State);
    auto command_palette_state =
        push_struct(client->project_arena, Command_Palette_State);

    utilities_layer_state->command_palette_state = command_palette_state;

    Command_Palette_Component command_palette_components[] = {
        {
            STR_LIT("component.theme_selector"),
            STR_LIT("Theme Selector"),
            STR_LIT("Browse and apply the built-in themes"),
            STR_LIT("theme colors style appearance ui"),
            theme_selector_component_on_open,
            theme_selector_component_render,
            &utilities_layer_state->theme_selector_component_state,
        },
        {
            STR_LIT("component.settings"),
            STR_LIT("Settings"),
            STR_LIT("Toggle editor utility settings"),
            STR_LIT("settings preferences demos debug palette"),
            nullptr,
            settings_component_render,
            nullptr,
        },
    };
    command_palette_init(command_palette_state,
                         command_palette_components,
                         (s32)ARRAY_COUNT(command_palette_components));

    // Create and register editor layer
    client->layers.init(client->project_arena);

    // Add layers
    client->layers.add(create_editor_layer(editor_layer_state));
    client->layers.add(create_utilities_layer(utilities_layer_state));

#ifdef DEBUG_BUILD
    auto debug_layer_state =
        push_struct(client->project_arena, Debug_Layer_State);
    client->layers.add(create_debug_layer(debug_layer_state));
#endif

    // Set titlebar content callback and logo asset
    client->titlebar_content_callback = client_titlebar_content_callback;
    client->logo_asset_name           = "voltrum_icon";

    return true;
}
