#pragma once

#include "client_types.hpp"
#include "core/application.hpp"
#include "core/logger.hpp"
#include "core/thread_context.hpp"
#include "defines.hpp"

#ifdef DEBUG_BUILD
#    include "memory/arena_debug.hpp"
#endif

// Client must implement this function to initialize their state
extern b8         create_client(Client *client_state);
extern App_Config request_client_config();

int
main()
{
#ifdef DEBUG_BUILD
    arena_debug_init();
#endif

    Thread_Context *thread_context = thread_context_allocate();
    thread_context->thread_name    = "Application main thread";
    thread_context_select(thread_context);

    auto config = request_client_config();

    // Initialize application with client state
    Client *client = application_init(&config);

    ENSURE(client);

    // Let client initialize its state and configuration
    if (!create_client(client))
    {
        CORE_FATAL("Failed to initialize client");
        return -1;
    }

    CORE_INFO("Client application initialized successfully");

    // Run the application
    application_run();

    thread_context_release(thread_context);

#ifdef DEBUG_BUILD
    arena_debug_shutdown();
#endif

    return 0;
}
