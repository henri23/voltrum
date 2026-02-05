#include "application.hpp"

#include "client_types.hpp"
#include "core/absolute_clock.hpp"
#include "core/asserts.hpp"
#include "core/frame_context.hpp"
#include "core/logger.hpp"
#include "core/thread_context.hpp"
#include "events/events.hpp"
#include "input/input.hpp"
#include "math/math.hpp"
#include "memory/arena.hpp"
#include "platform/platform.hpp"
#include "renderer/renderer_frontend.hpp"

#include "resources/resource_types.hpp"
#include "systems/geometry_system.hpp"
#include "systems/material_system.hpp"
#include "systems/resource_system.hpp"
#include "systems/texture_system.hpp"
#include "ui/ui.hpp"
#include "utils/string.hpp"

// Application configuration
constexpr u32 TARGET_FPS        = 120;
constexpr f64 TARGET_FRAME_TIME = 1 / (f64)TARGET_FPS;

struct Engine_State
{
    // Configuration requested by client
    App_Config config;

    // Arenas
    Arena *persistent_arena;
    Arena *frame_arenas[2];

    // Client state management
    Arena  *client_arena;
    Client *client;

    // Statuses
    b8  is_running;
    b8  is_suspended;
    u16 width;
    u16 height;

    Absolute_Clock clock;

    Event_Queue *event_queue;

    // Subsystem state
    Platform_State        *platform;
    Input_State           *inputs;
    Event_State           *events;
    Resource_System_State *resources;
    Renderer_System_State *renderer;
    Texture_System_State  *textures;
    Material_System_State *materials;
    Geometry_System_State *geometries;
    UI_State              *ui;

    Geometry *test_geometry;
    f32       cube_rotation;
};

// Internal pointer to application state for easy access
internal_var Engine_State *engine_state = nullptr;

INTERNAL_FUNC void
application_set_window_icon()
{
    Scratch_Arena scratch = scratch_begin(nullptr, 0);

    Resource icon_resource = {};
    if (resource_system_load(scratch.arena,
                             "voltrum",
                             Resource_Type::ICON,
                             &icon_resource))
    {
        Image_Resource_Data *icon_data =
            static_cast<Image_Resource_Data *>(icon_resource.data);

        platform_set_window_icon(engine_state->platform,
                                 icon_data->pixels,
                                 icon_data->width,
                                 icon_data->height);

        CORE_DEBUG("Window icon set successfully");
    }
    else
    {
        CORE_WARN("Failed to load window icon");
    }

    scratch_end(scratch);
}

INTERNAL_FUNC b8
app_escape_key_callback(const Event *event)
{
    if (event->key.key_code == Key_Code::ESCAPE && !event->key.repeat)
    {
        CORE_INFO("ESC key pressed - closing application");
        engine_state->is_running = false;
    }
    return false; // Don't consume, let other callbacks process
}

INTERNAL_FUNC b8
app_on_debug_event(const Event *event)
{
    const char *names[3] = {"metal", "space_parallax", "yellow_track"};

    local_persist s8 choice = 0;

    const char *old_name = names[choice];

    choice++;
    choice %= 3;

    if (engine_state->test_geometry)
    {
        engine_state->test_geometry->material->diffuse_map.texture =
            texture_system_acquire(names[choice], true);
        if (!engine_state->test_geometry->material->diffuse_map.texture)
        {
            CORE_WARN("event_on_debug_event not nexture! using default");
            engine_state->test_geometry->material->diffuse_map.texture =
                texture_system_get_default_texture();
        }

        texture_system_release(old_name);
    }

    return true;
}

void
application_get_framebuffer_size(u32 *width, u32 *height)
{
    *width  = engine_state->width;
    *height = engine_state->height;
}

INTERNAL_FUNC b8
app_on_resized_callback(const Event *event)
{

    if (event->window_resize.width != engine_state->width ||
        event->window_resize.height != engine_state->height)
    {

        engine_state->width  = event->window_resize.width;
        engine_state->height = event->window_resize.height;

        // Handle minimization
        if (engine_state->width == 0 || engine_state->height == 0)
        {
            CORE_INFO("Windows minimized, suspending application.");
            engine_state->is_suspended = true;
            return true;
        }
        else
        {
            if (engine_state->is_suspended)
            {
                CORE_INFO("Window restored, resuming application");
                engine_state->is_suspended = false;
            }

            engine_state->client->on_resize(engine_state->client,
                                            engine_state->width,
                                            engine_state->height);

            renderer_on_resize(engine_state->width, engine_state->height);
        }
    }
    return false;
}

Client *
application_init(App_Config *config)
{
    RUNTIME_ASSERT_MSG(
        config,
        "application_init - Client configuration cannot be null");

    // Setup core application arena
    Arena *persistent_arena = arena_create();
    engine_state            = push_struct(persistent_arena, Engine_State);

    // Setup client app arena
    Arena *client_arena        = arena_create();
    engine_state->client_arena = client_arena;

    // Core state
    engine_state->persistent_arena = persistent_arena;
    engine_state->config           = *config;

    // Client state
    Client *client_state = push_struct(engine_state->client_arena, Client);

    engine_state->client             = client_state;
    engine_state->client->mode_arena = engine_state->client_arena;

    if (!log_init())
    {
        CORE_FATAL("Failed to initialize log subsystem");
        return nullptr;
    }

    // Platform layer
    engine_state->platform = platform_init(engine_state->persistent_arena,
                                           config->name,
                                           config->width,
                                           config->height);
    ENSURE(engine_state->platform);

    // Event system, queue and input
    engine_state->events = events_init(engine_state->persistent_arena);
    ENSURE(engine_state->events);

    engine_state->event_queue =
        event_queue_create(engine_state->persistent_arena);

    engine_state->inputs = input_init(engine_state->persistent_arena);
    ENSURE(engine_state->inputs);

    // Resource & texture systems
    Resource_System_Config resource_config = {};

#ifdef DEBUG_BUILD
    resource_config.asset_base_path = "../assets";
#elif RELEASE_BUILD
    resource_config.asset_base_path = "../assets";
#endif

    engine_state->resources =
        resource_system_init(engine_state->persistent_arena, resource_config);
    ENSURE(engine_state->resources);

    // Set window icon using cross-platform SDL method
    application_set_window_icon();

    engine_state->renderer = renderer_init(engine_state->persistent_arena,
                                           engine_state->platform,
                                           config->name);
    ENSURE(engine_state->renderer);

    Texture_System_Config texture_config = {1024};
    engine_state->textures =
        texture_system_init(engine_state->persistent_arena, texture_config);

    ENSURE(engine_state->textures);

    Material_System_Config material_config = {4096};
    engine_state->materials =
        material_system_init(engine_state->persistent_arena, material_config);
    ENSURE(engine_state->materials);

    Geometry_System_Config geometry_config = {4096};
    engine_state->geometries =
        geometry_system_init(engine_state->persistent_arena, geometry_config);
    ENSURE(engine_state->geometries);

    // TODO: Temp
    // internal_state->test_geometry = geometry_system_get_default();

    // -- PLANE GEOMETRY (commented out) --
    // Geometry_Config g_config = geometry_system_generate_plane_config(10.0f,
    //     5.0f,
    //     5,
    //     5,
    //     5.0f,
    //     2.0f,
    //     "test geometry",
    //     "test_material");

    // -- CUBE GEOMETRY (delete later) --
    Geometry_Config g_config;
    g_config.vertex_count = 24;
    g_config.vertices =
        push_array(engine_state->persistent_arena, vertex_3d, 24);
    g_config.index_count = 36;
    g_config.indices     = push_array(engine_state->persistent_arena, u32, 36);

    g_config.name = const_str_from_cstr<GEOMETRY_NAME_MAX_LENGTH>("test_cube");
    g_config.material_name =
        const_str_from_cstr<MATERIAL_NAME_MAX_LENGTH>("test_material");

    f32 s = 1.0f; // half-size

    // Vertices wound CLOCKWISE when viewed from outside each face
    // (CW = front face with negative viewport height in Vulkan)

    // Front face (+Z) - looking from +Z toward origin
    g_config.vertices[0] = {{-s, -s, s}, {0, 1}}; // bottom-left
    g_config.vertices[1] = {{-s, s, s}, {0, 0}};  // top-left
    g_config.vertices[2] = {{s, s, s}, {1, 0}};   // top-right
    g_config.vertices[3] = {{s, -s, s}, {1, 1}};  // bottom-right

    // Back face (-Z) - looking from -Z toward origin
    g_config.vertices[4] = {{s, -s, -s}, {0, 1}};  // bottom-left
    g_config.vertices[5] = {{s, s, -s}, {0, 0}};   // top-left
    g_config.vertices[6] = {{-s, s, -s}, {1, 0}};  // top-right
    g_config.vertices[7] = {{-s, -s, -s}, {1, 1}}; // bottom-right

    // Top face (+Y) - looking from +Y toward origin
    g_config.vertices[8]  = {{-s, s, s}, {0, 1}};  // bottom-left
    g_config.vertices[9]  = {{-s, s, -s}, {0, 0}}; // top-left
    g_config.vertices[10] = {{s, s, -s}, {1, 0}};  // top-right
    g_config.vertices[11] = {{s, s, s}, {1, 1}};   // bottom-right

    // Bottom face (-Y) - looking from -Y toward origin
    g_config.vertices[12] = {{-s, -s, -s}, {0, 1}}; // bottom-left
    g_config.vertices[13] = {{-s, -s, s}, {0, 0}};  // top-left
    g_config.vertices[14] = {{s, -s, s}, {1, 0}};   // top-right
    g_config.vertices[15] = {{s, -s, -s}, {1, 1}};  // bottom-right

    // Right face (+X) - looking from +X toward origin
    g_config.vertices[16] = {{s, -s, s}, {0, 1}};  // bottom-left
    g_config.vertices[17] = {{s, s, s}, {0, 0}};   // top-left
    g_config.vertices[18] = {{s, s, -s}, {1, 0}};  // top-right
    g_config.vertices[19] = {{s, -s, -s}, {1, 1}}; // bottom-right

    // Left face (-X) - looking from -X toward origin
    g_config.vertices[20] = {{-s, -s, -s}, {0, 1}}; // bottom-left
    g_config.vertices[21] = {{-s, s, -s}, {0, 0}};  // top-left
    g_config.vertices[22] = {{-s, s, s}, {1, 0}};   // top-right
    g_config.vertices[23] = {{-s, -s, s}, {1, 1}};  // bottom-right

    // Indices: two triangles per face (0,1,2) and (0,2,3)
    u32 base_idx[] = {0, 1, 2, 0, 2, 3};
    for (u32 f = 0; f < 6; ++f)
    {
        for (u32 i = 0; i < 6; ++i)
        {
            g_config.indices[f * 6 + i] = f * 4 + base_idx[i];
        }
    }
    engine_state->cube_rotation = 0.0f;
    // -- END CUBE GEOMETRY --

    engine_state->test_geometry =
        geometry_system_acquire_by_config(g_config, true);

    // Vertices and indices are arena-allocated and will be freed with the arena
    // TODO: Temp

    // Register application ESC key handler with HIGH priority to always work
    events_register_callback(Event_Type::KEY_PRESSED,
                             app_escape_key_callback,
                             Event_Priority::HIGH);

    events_register_callback(Event_Type::WINDOW_RESIZED,
                             app_on_resized_callback,
                             Event_Priority::HIGH);

    events_register_callback(Event_Type::DEBUG0, app_on_debug_event);

    engine_state->is_running   = false;
    engine_state->is_suspended = false;

    CORE_INFO("All subsystems initialized correctly.");

    return engine_state->client;
}

void
application_run()
{
    if (!engine_state)
    {
        CORE_FATAL("Application not initialized");
        return;
    }

    engine_state->ui = ui_init(engine_state->persistent_arena,
                               &engine_state->client->layers,
                               UI_Theme::CATPPUCCIN,
                               engine_state->client->menu_callback,
                               engine_state->config.name,
                               engine_state->platform,
                               engine_state->client->state);

    ENSURE(engine_state->ui);

    engine_state->is_running = true;

    absolute_clock_start(&engine_state->clock);
    absolute_clock_update(&engine_state->clock);

    // Call client initialize if provided
    if (engine_state->client->initialize)
    {
        if (!engine_state->client->initialize(engine_state->client))
        {
            CORE_ERROR("Client initialization failed");
            return;
        }
    }

    // Frame rate limiting variables
    f64 last_time = platform_get_absolute_time();

    Frame_Context frame_ctx = {};

    // MAIN LOOP
    while (engine_state->is_running)
    {
        Scratch_Arena frame_scratch = scratch_begin(nullptr, 0);

        frame_ctx.frame_arena = frame_scratch.arena;
        frame_ctx.event_queue = engine_state->event_queue;

        if (!platform_message_pump(&frame_ctx))
        {
            engine_state->is_running = false;
        }

        // TODO: Think whether message queue events need to persist between
        // frames
        event_queue_flush(frame_ctx.event_queue);

        // Frame
        if (!engine_state->is_suspended)
        {

            f64 frame_start_time = platform_get_absolute_time();
            f64 delta_time       = frame_start_time - last_time;
            last_time            = frame_start_time;

            frame_ctx.delta_t = delta_time;

            // DEBUG: Check frame timing consistency
            // CORE_DEBUG("delta: %.4f ms", delta_time * 1000.0);

            if (engine_state->client->update)
            {
                if (!engine_state->client->update(engine_state->client,
                                                  &frame_ctx))
                {
                    CORE_FATAL("Client update failed. Aborting...");
                    engine_state->is_running = false;
                }
            }

            if (engine_state->client->render)
            {
                if (!engine_state->client->render(engine_state->client,
                                                  &frame_ctx))
                {
                    CORE_FATAL("Client render failed. Aborting...");
                    engine_state->is_running = false;
                }
            }

            Render_Context *packet =
                push_struct(frame_ctx.frame_arena, Render_Context);

            // TODO: temp - viewport geometry
            Geometry_Render_Data test_render;
            test_render.geometry = engine_state->test_geometry;
            // Rotate cube over time
            engine_state->cube_rotation += delta_time;
            test_render.model =
                mat4_euler_xyz(engine_state->cube_rotation,
                               engine_state->cube_rotation * 0.7f,
                               0.0f);

            packet->geometry_count = 1;
            packet->geometries     = &test_render;

            ui_update_layers(engine_state->ui, &frame_ctx);

            packet->ui_data.draw_list =
                ui_draw_layers(engine_state->ui, &frame_ctx);

            if (!renderer_draw_frame(&frame_ctx, packet))
            {
                engine_state->is_running = false;
            }

            // Frame rate limiting
            f64 frame_end_time = platform_get_absolute_time();
            f64 frame_duration = frame_end_time - frame_start_time;

            if (frame_duration < TARGET_FRAME_TIME)
            {
                u64 sleep_ms =
                    (u64)((TARGET_FRAME_TIME - frame_duration) * 1000.0);

                if (sleep_ms > 0)
                {
                    platform_sleep(sleep_ms);
                }
            }

            // Update input state each frame
            input_update();
        }

        scratch_end(frame_scratch);
    }

    application_shutdown();
}

void
application_shutdown()
{
    ENSURE(engine_state);

    CORE_DEBUG("Shutting down UI subsystem...");
    ui_shutdown_layers(engine_state->ui);

    // Call client shutdown if provided
    if (engine_state->client->shutdown)
    {
        CORE_DEBUG("Shutting down client...");
        engine_state->client->shutdown(engine_state->client);
    }

    arena_release(engine_state->client_arena);

    CORE_DEBUG("Shutting down material subsystem...");
    material_system_shutdown();

    CORE_DEBUG("Shutting down texture subsystem...");
    texture_system_shutdown();

    CORE_DEBUG("Shutting down platform subsystem...");
    platform_shutdown(engine_state->platform);

    CORE_INFO("All subsystems shut down correctly.");

    CORE_DEBUG("Shutting down logging subsystem...");
    log_shutdown();

    // Free application state
    arena_release(engine_state->persistent_arena);
}
