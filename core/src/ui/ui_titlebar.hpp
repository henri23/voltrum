#pragma once

#include "defines.hpp"

struct UI_Context;

const f32 TITLEBAR_HEIGHT = 54.0f;

VOLTRUM_API void ui_titlebar_setup(UI_Context* context, const char* app_name);

VOLTRUM_API void ui_titlebar_draw(UI_Context* context);

VOLTRUM_API b8 ui_is_titlebar_hovered(UI_Context* context);
