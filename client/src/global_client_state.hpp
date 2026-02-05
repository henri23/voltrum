#pragma once

#include "defines.hpp"

// Client-specific state structure
struct Global_Client_State
{
    b8 initialized;

    // Statuses of layers
    b8 is_debug_layer_visible;

    // Statuses of windows
    b8 is_imgui_demo_visible;
    b8 is_implot_demo_visible;
};
