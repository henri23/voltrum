#pragma once

#include "defines.hpp"

struct UI_Layer {
    void* state; // Layer state used in client, but managed in core

    void (*on_attach)(UI_Layer* self);
    void (*on_detach)(UI_Layer* self);

    b8 (*on_update)(UI_Layer* self, f32 delta_time);
    b8 (*on_render)(UI_Layer* self, f32 delta_time);
    // TODO: Enable later
    // b8 (*on_event)(UI_Layer* self, Event event);
};

typedef void (*PFN_menu_callback)();
