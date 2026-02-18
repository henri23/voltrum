#pragma once

#include "defines.hpp"
#include "utils/string.hpp"

#include <ui/ui_themes.hpp>

struct Global_Client_State;

void settings_component_render(void                   *component_state,
                               Global_Client_State    *global_state,
                               const UI_Theme_Palette &palette,
                               String                  filter,
                               b8                     *request_close);
