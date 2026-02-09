#pragma once

#include "defines.hpp"
#include <ui/ui_themes.hpp>

constexpr f32 TOOLBAR_SLOT_SIZE          = 36.0f;
constexpr f32 TOOLBAR_SLOT_GAP           = 8.0f;
constexpr f32 TOOLBAR_CONTAINER_PAD      = 2.0f;
constexpr f32 TOOLBAR_CONTAINER_HEIGHT   = TOOLBAR_SLOT_SIZE +
                                           TOOLBAR_CONTAINER_PAD * 2.0f;

void toolbar_component_render(s32                    *active_tool_index,
                              f32                     emphasis,
                              const UI_Theme_Palette &palette);
