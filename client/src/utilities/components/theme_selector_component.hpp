#pragma once

#include "defines.hpp"
#include "utils/string.hpp"

#include <ui/ui_themes.hpp>

struct Global_Client_State;

struct Theme_Selector_Component_State
{
    s32 selection;
};

void theme_selector_component_on_open(void                *component_state,
                                      Global_Client_State *global_state);
void theme_selector_component_render(void                   *component_state,
                                     Global_Client_State    *global_state,
                                     const UI_Theme_Palette &palette,
                                     String                  filter,
                                     b8                     *request_close);
