#pragma once

#include <ui/ui_types.hpp>

// Client titlebar content callback - renders menus in the titlebar content area
void client_titlebar_content_callback(void                   *client_state,
                                      vec2                    content_area_min,
                                      vec2                    content_area_max,
                                      const UI_Theme_Palette *palette);
