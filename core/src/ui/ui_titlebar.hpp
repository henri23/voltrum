#pragma once

#include "defines.hpp"
#include "utils/string.hpp"

const f32 TITLEBAR_HEIGHT = 50.0f;

VOLTRUM_API void ui_titlebar_setup(
    struct UI_State *state,
    String           logo_asset_name
);

VOLTRUM_API void ui_titlebar_draw(struct UI_State *state);
