#pragma once

#include "defines.hpp"
#include "utils/string.hpp"

struct UI_Theme_Palette;
struct Global_Client_State;

constexpr u64 COMMAND_PALETTE_FILTER_MAX_LENGTH   = 160;
constexpr s32 COMMAND_PALETTE_MAX_COMPONENT_COUNT = 64;

typedef void (*PFN_Command_Palette_Component_On_Open)(
    void                *component_state,
    Global_Client_State *global_state);

typedef void (*PFN_Command_Palette_Component_Render)(
    void                   *component_state,
    Global_Client_State    *global_state,
    const UI_Theme_Palette &palette,
    String                  filter,
    b8                     *request_close);

struct Command_Palette_Component
{
    String id;
    String label;
    String description;
    String keywords;

    PFN_Command_Palette_Component_On_Open on_open;
    PFN_Command_Palette_Component_Render  on_render;
    void                                 *component_state;
};

struct Command_Palette_State
{
    f32 intro_t;
    s32 selection;
    s32 active_component_index;
    b8  focus_filter;

    char command_filter[COMMAND_PALETTE_FILTER_MAX_LENGTH];

    Command_Palette_Component components[COMMAND_PALETTE_MAX_COMPONENT_COUNT];
    s32                       component_count;
};

void command_palette_init(Command_Palette_State           *state,
                          const Command_Palette_Component *components,
                          s32                              component_count);

void command_palette_component_render(Command_Palette_State *state,
                                      Global_Client_State   *global_state,
                                      const struct UI_Theme_Palette &palette,
                                      f32 delta_time);
