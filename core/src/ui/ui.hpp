#pragma once

#include "data_structures/dynamic_array.hpp"
#include "memory/arena.hpp"
#include "ui_themes.hpp"
#include "ui_types.hpp"
#include "utils/string.hpp"

VOLTRUM_API UI_State *
ui_init(Arena                        *arena,
        Dynamic_Array<UI_Layer>      *layers,
        UI_Theme                      theme,
        PFN_titlebar_content_callback titlebar_content_callback,
        String                        logo_asset_name,
        struct Platform_State        *plat_state,
        void                         *global_client_state);

VOLTRUM_API void ui_shutdown_layers(UI_State *state);

VOLTRUM_API void ui_update_layers(UI_State             *state,
                                  struct Frame_Context *frame_ctx);

VOLTRUM_API struct ImDrawData *ui_draw_layers(UI_State             *state,
                                              struct Frame_Context *frame_ctx);

// Consolidated theme API. Pass either pointer, or both, depending on the
// operation.
VOLTRUM_API void ui_set_theme_state(const UI_Theme         *theme,
                                    const UI_Theme_Palette *palette);

VOLTRUM_API void ui_get_theme_state(UI_Theme         *out_theme,
                                    UI_Theme_Palette *out_palette);

VOLTRUM_API void ui_set_theme(UI_Theme theme);

VOLTRUM_API UI_Theme ui_get_current_theme();

VOLTRUM_API void ui_set_theme_palette(const UI_Theme_Palette *palette);

VOLTRUM_API void ui_get_theme_palette(UI_Theme_Palette *out_palette);
