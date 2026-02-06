#pragma once

#include "defines.hpp"

struct UI_State;

const f32 TITLEBAR_HEIGHT = 54.0f;

VOLTRUM_API void ui_titlebar_setup(
    UI_State   *state,
    const char *logo_asset_name
);

VOLTRUM_API void ui_titlebar_draw(UI_State *state);
