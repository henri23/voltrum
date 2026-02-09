#pragma once

#include "defines.hpp"
#include "utils/string.hpp"

constexpr u64 COMMAND_PALETTE_ID_MAX_LENGTH          = 96;
constexpr u64 COMMAND_PALETTE_LABEL_MAX_LENGTH       = 96;
constexpr u64 COMMAND_PALETTE_DESCRIPTION_MAX_LENGTH = 192;
constexpr u64 COMMAND_PALETTE_KEYWORDS_MAX_LENGTH    = 192;
constexpr u64 COMMAND_PALETTE_FILTER_MAX_LENGTH      = 160;
constexpr s32 COMMAND_PALETTE_MAX_COMMAND_COUNT      = 256;

typedef void (*PFN_Command_Palette_Execute)(void *global_state,
                                            void *user_data);
typedef String (*PFN_Command_Palette_Resolve_Description)(
    struct Arena *arena,
    void         *global_state,
    void         *user_data,
    String        base_description);

struct Command_Palette_Registered_Command
{
    Const_String<COMMAND_PALETTE_ID_MAX_LENGTH>          id;
    Const_String<COMMAND_PALETTE_ID_MAX_LENGTH>          parent_id;
    Const_String<COMMAND_PALETTE_LABEL_MAX_LENGTH>       label;
    Const_String<COMMAND_PALETTE_DESCRIPTION_MAX_LENGTH> description;
    Const_String<COMMAND_PALETTE_KEYWORDS_MAX_LENGTH>    keywords;

    PFN_Command_Palette_Execute             on_execute;
    PFN_Command_Palette_Resolve_Description resolve_description;
    void                                   *user_data;
    b8                                      close_on_execute;
};

struct Command_Palette_State
{
    b8 initialized;

    f32 intro_t;
    s32 selection;
    b8  focus_filter;

    char command_filter[COMMAND_PALETTE_FILTER_MAX_LENGTH];
    Const_String<COMMAND_PALETTE_ID_MAX_LENGTH> active_parent_id;

    Command_Palette_Registered_Command
        commands[COMMAND_PALETTE_MAX_COMMAND_COUNT];
    s32 command_count;
};

struct Command_Palette_Command_Definition
{
    String id;
    String parent_id;
    String label;
    String description;
    String keywords;

    PFN_Command_Palette_Execute             on_execute;
    PFN_Command_Palette_Resolve_Description resolve_description;
    void                                   *user_data;
    b8                                      close_on_execute;
};

void command_palette_init(Command_Palette_State *state);
void command_palette_reset_state(Command_Palette_State *state);
void command_palette_clear_registry(Command_Palette_State *state);
b8   command_palette_register(Command_Palette_State                    *state,
                              const Command_Palette_Command_Definition *command);

void command_palette_component_render(Command_Palette_State      *state,
                                      struct Global_Client_State *global_state,
                                      const struct UI_Theme_Palette &palette,
                                      f32 delta_time);
