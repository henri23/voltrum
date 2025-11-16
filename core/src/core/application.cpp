#include "application.hpp"

#include "client_types.hpp"
#include "core/absolute_clock.hpp"
#include "core/asserts.hpp"
#include "core/logger.hpp"
#include "events/events.hpp"
#include "input/input.hpp"
#include "memory/memory.hpp"
#include "platform/platform.hpp"
#include "renderer/renderer_frontend.hpp"
#include "ui/ui.hpp"

// Application configuration
constexpr u32 TARGET_FPS = 140;
constexpr f64 TARGET_FRAME_TIME = 1 / (f64)TARGET_FPS;

struct Internal_App_State {
    Client* client;
    b8 is_running;
    b8 is_suspended;
    u16 width;
    u16 height;

    Platform_State plat_state;
    Absolute_Clock clock;
};

// Internal pointer to application state for easy access
internal_variable Internal_App_State* internal_state = nullptr;

INTERNAL_FUNC b8 app_escape_key_callback(const Event* event) {
    if (event->key.key_code == Key_Code::ESCAPE && !event->key.repeat) {
        CORE_INFO("ESC key pressed - closing application");
        internal_state->is_running = false;
    }
    return false; // Don't consume, let other callbacks process
}

void application_get_framebuffer_size(u32* width, u32* height) {
    *width = internal_state->width;
    *width = internal_state->width;
}

INTERNAL_FUNC b8 app_on_resized_callback(const Event* event) {

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

b8 application_init(Client* client_state) {
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
        static_cast<Internal_App_State*>(client_state->internal_app_state);

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

    if (!renderer_startup(client_state->config.name)) {
        CORE_FATAL("Failed to initialize renderer");
        return false;
    }

    // Initialize UI with configuration from client
    if (!ui_initialize(client_state->config.theme,
            &client_state->layers,
            client_state->menu_callback,
            client_state->config.name,
            internal_state->plat_state.window)) {
        CORE_FATAL("Failed to initialize UI subsystem");
        return false;
    }

    // Register application ESC key handler with HIGH priority to always work
    events_register_callback(Event_Type::KEY_PRESSED,
        app_escape_key_callback,
        Event_Priority::HIGH);

    events_register_callback(Event_Type::WINDOW_RESIZED,
        app_on_resized_callback,
        Event_Priority::HIGH);

    internal_state->is_running = false;
    internal_state->is_suspended = false;

    CORE_INFO("All subsystems initialized correctly.");

    CORE_DEBUG(
        memory_get_current_usage()); // WARN: Memory leak because the heap
                                     // allocated string must be deallocated

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
    ui_shutdown();

    CORE_DEBUG("Shutting down renderer subsystem...");
    renderer_shutdown();

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
