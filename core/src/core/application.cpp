#include "application.hpp"

#include "client_types.hpp"
#include "core/absolute_clock.hpp"
#include "core/asserts.hpp"
#include "core/logger.hpp"
#include "events/events.hpp"
#include "input/input.hpp"
#include "math/math.hpp"
#include "memory/memory.hpp"
#include "platform/platform.hpp"
#include "renderer/renderer_frontend.hpp"

#include "resources/resource_types.hpp"
#include "systems/geometry_system.hpp"
#include "systems/material_system.hpp"
#include "systems/resource_system.hpp"
#include "systems/texture_system.hpp"
#include "ui/ui.hpp"

INTERNAL_FUNC void application_set_window_icon() {
    Resource icon_resource = {};
    if (resource_system_load("voltrum", Resource_Type::ICON, &icon_resource)) {
        Image_Resource_Data *icon_data =
            static_cast<Image_Resource_Data *>(icon_resource.data);

        platform_set_window_icon(icon_data->pixels,
            icon_data->width,
            icon_data->height);

        resource_system_unload(&icon_resource);
        CORE_DEBUG("Window icon set successfully");
    } else {
        CORE_WARN("Failed to load window icon");
    }
}

// Application configuration
constexpr u32 TARGET_FPS = 140;
constexpr f64 TARGET_FRAME_TIME = 1 / (f64)TARGET_FPS;

struct Internal_App_State {
    Client *client;
    b8 is_running;
    b8 is_suspended;
    u16 width;
    u16 height;

    Platform_State plat_state;
    Absolute_Clock clock;

    UI_Context ui_context;

    Geometry *test_geometry;
};

// Internal pointer to application state for easy access
internal_var Internal_App_State *internal_state = nullptr;

INTERNAL_FUNC b8 app_escape_key_callback(const Event *event) {
    if (event->key.key_code == Key_Code::ESCAPE && !event->key.repeat) {
        CORE_INFO("ESC key pressed - closing application");
        internal_state->is_running = false;
    }
    return false; // Don't consume, let other callbacks process
}

INTERNAL_FUNC b8 app_on_debug_event(const Event *event) {
    const char *names[3] = {"metal", "space_parallax", "yellow_track"};

    local_persist s8 choice = 0;

    const char *old_name = names[choice];

    choice++;
    choice %= 3;

    if (internal_state->test_geometry) {
        internal_state->test_geometry->material->diffuse_map.texture =
            texture_system_acquire(names[choice], true);
        if (!internal_state->test_geometry->material->diffuse_map.texture) {
            CORE_WARN("event_on_debug_event not nexture! using default");
            internal_state->test_geometry->material->diffuse_map.texture =
                texture_system_get_default_texture();
        }

        texture_system_release(old_name);
    }

    return true;
}

void application_get_framebuffer_size(u32 *width, u32 *height) {
    *width = internal_state->width;
    *height = internal_state->height;
}

INTERNAL_FUNC b8 app_on_resized_callback(const Event *event) {

    if (event->window_resize.width != internal_state->width ||
        event->window_resize.height != internal_state->height) {

        internal_state->width = event->window_resize.width;
        internal_state->height = event->window_resize.height;

        // Handle minimization
        if (internal_state->width == 0 || internal_state->height == 0) {
            CORE_INFO("Windows minimized, suspending application.");
            internal_state->is_suspended = true;
            return true;
        } else {
            if (internal_state->is_suspended) {
                CORE_INFO("Window restored, resuming application");
                internal_state->is_suspended = false;
            }

            internal_state->client->on_resize(internal_state->client,
                internal_state->width,
                internal_state->height);

            renderer_on_resize(internal_state->width, internal_state->height);
        }
    }
    return false;
}

b8 application_init(Client *client_state) {
    RUNTIME_ASSERT_MSG(client_state, "Client state cannot be null");

    // Protect against multiple initialization
    if (client_state->internal_app_state != nullptr) {
        CORE_ERROR("Application already initialized");
        return false;
    }

    // Allocate application state
    client_state->internal_app_state =
        memory_allocate(sizeof(Internal_App_State), Memory_Tag::APPLICATION);

    internal_state =
        static_cast<Internal_App_State *>(client_state->internal_app_state);

    internal_state->client = client_state;

    if (!log_init()) {
        CORE_FATAL("Failed to initialize log subsystem");
        return false;
    }

    if (!platform_startup(&internal_state->plat_state,
            client_state->config.name,
            client_state->config.width,
            client_state->config.height)) {
        CORE_FATAL("Failed to initialize platform subsystem");
        return false;
    }

    // Initialize event and input systems
    events_initialize();
    input_initialize();

    Resource_System_Config resource_config = {32};

#ifdef DEBUG_BUILD
    resource_config.asset_base_path = "../assets";
#elif RELEASE_BUILD
    resource_config.asset_base_path = "../assets";
#endif

    if (!resource_system_init(resource_config)) {
        CORE_FATAL("Failed to initialize resource system");
        return false;
    }

    // Set window icon using cross-platform SDL method
    application_set_window_icon();

    if (!renderer_startup(client_state->config.name)) {
        CORE_FATAL("Failed to initialize renderer");
        return false;
    }

    Texture_System_Config config = {16384};
    if (!texture_system_init(config)) {
        CORE_FATAL("Failed to initialize texture system");
        return false;
    }

    Material_System_Config material_config = {4096};
    if (!material_system_init(material_config)) {
        CORE_FATAL("Failed to initialize material system");
        return false;
    }

    Geometry_System_Config geometry_config = {4096};
    if (!geometry_system_init(geometry_config)) {
        CORE_FATAL("Failed to initialize geometry system");
        return false;
    }

    // TODO: Temp
    // internal_state->test_geometry = geometry_system_get_default();
    // Load a plane config, and load a geometry from it
    Geometry_Config g_config = geometry_system_generate_plane_config(10.0f,
        5.0f,
        5,
        5,
        5.0f,
        2.0f,
        "test geometry",
        "test_material");

    internal_state->test_geometry =
        geometry_system_acquire_by_config(g_config, true);

    memory_deallocate(g_config.vertices,
        sizeof(vertex_3d) * g_config.vertex_count,
        Memory_Tag::ARRAY);

    memory_deallocate(g_config.indices,
        sizeof(u32) * g_config.index_count,
        Memory_Tag::ARRAY);
    // TODO: Temp

    if (!ui_initialize(&internal_state->ui_context,
            internal_state->client->layers.data,
            internal_state->client->layers.length,
            UI_Theme::CATPPUCCIN,
            client_state->menu_callback,
            client_state->config.name,
            internal_state->plat_state.window)) {
        CORE_FATAL("Failed to initiliaze ui system");
        return false;
    }

    // Register application ESC key handler with HIGH priority to always work
    events_register_callback(Event_Type::KEY_PRESSED,
        app_escape_key_callback,
        Event_Priority::HIGH);

    events_register_callback(Event_Type::WINDOW_RESIZED,
        app_on_resized_callback,
        Event_Priority::HIGH);

    events_register_callback(Event_Type::DEBUG0, app_on_debug_event);

    internal_state->is_running = false;
    internal_state->is_suspended = false;

    CORE_INFO("All subsystems initialized correctly.");

    char memory_usage_str[5000];
    memory_get_current_usage(memory_usage_str);

    CORE_DEBUG(memory_usage_str);

    return true;
}

void application_run() {
    if (!internal_state) {
        CORE_FATAL("Application not initialized");
        return;
    }

    internal_state->is_running = true;

    absolute_clock_start(&internal_state->clock);
    absolute_clock_update(&internal_state->clock);

    // Call client initialize if provided
    if (internal_state->client->initialize) {
        if (!internal_state->client->initialize(internal_state->client)) {
            CORE_ERROR("Client initialization failed");
            return;
        }
    }

    // Frame rate limiting variables
    f64 last_time = platform_get_absolute_time();

    while (internal_state->is_running) {

        // Process platform events (will forward to UI via callback)
        if (!platform_message_pump()) {
            internal_state->is_running = false;
        }

        // Frame
        if (!internal_state->is_suspended) {

            f64 frame_start_time = platform_get_absolute_time();
            f64 delta_time = frame_start_time - last_time;

            if (internal_state->client->update) {
                if (!internal_state->client->update(internal_state->client,
                        delta_time)) {
                    CORE_FATAL("Client update failed. Aborting...");
                    internal_state->is_running = false;
                }
            }

            if (internal_state->client->render) {
                if (!internal_state->client->render(internal_state->client,
                        delta_time)) {
                    CORE_FATAL("Client render failed. Aborting...");
                    internal_state->is_running = false;
                }
            }

            Render_Packet packet;
            packet.delta_time = delta_time;

            // TODO: temp - viewport geometry
            Geometry_Render_Data test_render;
            test_render.geometry = internal_state->test_geometry;
            test_render.model = mat4_identity();

            packet.geometry_count = 1;
            packet.geometries = &test_render;

            ui_update_layers(&internal_state->ui_context,
                internal_state->client->layers.data,
                internal_state->client->layers.length,
                packet.delta_time);

            packet.ui_data.draw_list =
                ui_draw_layers(&internal_state->ui_context,
                    internal_state->client->layers.data,
                    internal_state->client->layers.length,
                    packet.delta_time);

            if (!renderer_draw_frame(&packet)) {
                internal_state->is_running = false;
            }

            // Frame rate limiting
            f64 frame_end_time = platform_get_absolute_time();
            f64 frame_duration = frame_end_time - frame_start_time;

            if (frame_duration < TARGET_FRAME_TIME) {
                u64 sleep_ms =
                    (u64)((TARGET_FRAME_TIME - frame_duration) * 1000.0);

                if (sleep_ms > 0) {
                    platform_sleep(sleep_ms);
                }
            }

            // Update input state each frame
            input_update();

            // Update last_time to include sleep for accurate frame timing
            last_time = platform_get_absolute_time();
        }
    }

    application_shutdown();
}

void application_shutdown() {
    if (!internal_state) {
        return;
    }

    CORE_INFO("Starting application shutdown...");

    // Call client shutdown if provided
    if (internal_state->client->shutdown) {
        CORE_DEBUG("Shutting down client...");

        internal_state->client->shutdown(internal_state->client);

        CORE_DEBUG("Client shutdown complete.");
    }

    CORE_DEBUG("Shutting down UI subsystem...");
    ui_shutdown(&internal_state->ui_context,
        internal_state->client->layers.data,
        internal_state->client->layers.length);

    CORE_DEBUG("Shutting down geometry subsystem...");
    geometry_system_shutdown();

    CORE_DEBUG("Shutting down material subsystem...");
    material_system_shutdown();

    CORE_DEBUG("Shutting down texture subsystem...");
    texture_system_shutdown();

    CORE_DEBUG("Shutting down renderer subsystem...");
    renderer_shutdown();

    CORE_DEBUG("Shutting down resource subsystem...");
    resource_system_shutdown();

    CORE_DEBUG("Shutting down input and event subsystems...");
    input_shutdown();
    events_shutdown();

    CORE_DEBUG("Shutting down platform subsystem...");
    platform_shutdown();

    CORE_INFO("All subsystems shut down correctly.");

    CORE_DEBUG("Shutting down logging subsystem...");
    log_shutdown();

    // Free application state
    memory_deallocate(internal_state,
        sizeof(Internal_App_State),
        Memory_Tag::APPLICATION);

    internal_state = nullptr;
}
