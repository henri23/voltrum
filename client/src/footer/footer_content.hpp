#pragma once

#include <ui/ui_types.hpp>

// Client footer content callback - renders status info in the footer area.
void client_footer_content_callback(void                   *client_state,
                                    vec2                    content_area_min,
                                    vec2                    content_area_max,
                                    const UI_Theme_Palette *palette);
