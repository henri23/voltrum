#pragma once

#include <ui/ui_types.hpp>

// Client titlebar content callback - renders menus in the titlebar content area
void client_titlebar_content_callback(
    void                          *client_state,
    const Titlebar_Content_Bounds &bounds,
    const UI_Theme_Palette        &palette
);
