#pragma once

#include "data_structures/dynamic_array.hpp"
#include "defines.hpp"
#include "ui/ui_themes.hpp"
#include "ui/ui_types.hpp"
#include "utils/string.hpp"

// Client configuration structure - client controls engine behavior
struct App_Config
{
    UI_Theme theme;
    String   name;
    u32      width;
    u32      height;
};

// Application structure - similar to Game struct in koala_engine
struct Client
{
    Arena *mode_arena;

    // Lifecycle callbacks - client implements these
    b8 (*initialize)(Client *);
    b8 (*update)(Client *, struct Frame_Context *);
    b8 (*render)(Client *, struct Frame_Context *);
    void (*on_resize)(Client *, u32 width, u32 height);
    void (*shutdown)(Client *);

    // Client-specific state
    Dynamic_Array<UI_Layer> layers;

    // Titlebar content callback and logo asset name
    PFN_titlebar_content_callback titlebar_content_callback;
    const char                   *logo_asset_name;

    void *state; // Client state
};
