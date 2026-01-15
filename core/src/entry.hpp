#pragma once

#include "client_types.hpp"
#include "core/application.hpp"
#include "core/logger.hpp"
#include "defines.hpp"

// Client must implement this function to initialize their state
extern b8 create_client(Client *client_state);

int main() {
    // Stack-allocate client state (following koala_engine pattern)
    Client client_state = {};

    // Let client initialize its state and configuration
    if (!create_client(&client_state)) {
        CORE_FATAL("Failed to initialize client");
        return -1;
    }

    // Validate required client configuration
    if (!client_state.config.name) {
        CORE_FATAL("Client must provide application name");
        return -1;
    }

    // Initialize application with client state
    if (!application_init(&client_state)) {
        CORE_FATAL("Failed to initialize application");
        return -1;
    }

    CORE_INFO("Client application initialized successfully");

    // Run the application
    application_run();

    return 0;
}
