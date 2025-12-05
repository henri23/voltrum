#pragma once

#include "defines.hpp"  // For b8 type

// #include "ui/ui.hpp"  // Commented out for UI rewrite

// Example application layer that demonstrates the viewport
b8 app_viewport_layer_initialize();
void app_viewport_layer_shutdown();
void app_viewport_layer_render(void* component_state);