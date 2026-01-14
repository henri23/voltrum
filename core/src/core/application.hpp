#pragma once

#include "defines.hpp"

// Forward declaration for client state (defined in client_types.hpp)
struct Client;

VOLTRUM_API void application_get_framebuffer_size(u32* width, u32* height);

// Initialize application with client state
VOLTRUM_API b8 application_init(Client* client_state);

// Run the main application loop
VOLTRUM_API void application_run();

// Shutdown and cleanup application
void application_shutdown();
