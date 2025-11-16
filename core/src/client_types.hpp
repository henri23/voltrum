#pragma once

#include "defines.hpp"
#include "ui/ui_types.hpp"
#include "ui/ui_themes.hpp"
#include "data_structures/auto_array.hpp"

// Client configuration structure - client controls engine behavior
struct App_Config {
	UI_Theme theme;
    const char* name;
    u32 width;
    u32 height;
};

// Application structure - similar to Game struct in koala_engine
struct Client {
    App_Config config;

    // Lifecycle callbacks - client implements these
    b8 (*initialize)(Client*);
    b8 (*update)(Client*, f32 delta_time);
    b8 (*render)(Client*, f32 delta_time);
    void (*on_resize)(Client*, u32 width, u32 height);
    void (*shutdown)(Client*);

    // Client-specific state
	Auto_Array<UI_Layer> layers;
	PFN_menu_callback menu_callback;
    
    void* state; // Client state

    // Internal engine state (opaque pointer managed by core)
    // Client cannot access this directly - only core can use this
    void* internal_app_state;
};
