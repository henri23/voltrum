#include "toolbar_component.hpp"
#include "core/asserts.hpp"

#include <ui/icons.hpp>
#include <ui/ui_widgets.hpp>
#include <utils/string.hpp>

internal_var const ui::icon_selector_item TOOLBAR_ITEMS[] = {
    {STR_LIT(ICON_FA_ARROW_POINTER), STR_LIT(""), STR_LIT("Select Tool")},
    {STR_LIT(ICON_FA_HAND), STR_LIT(""), STR_LIT("Pan Tool")},
    {STR_LIT(ICON_FA_PEN), STR_LIT(""), STR_LIT("Draw Tool")},
    {STR_LIT(ICON_FA_RULER), STR_LIT(""), STR_LIT("Measure Tool")},
    {STR_LIT(ICON_FA_MAGNIFYING_GLASS), STR_LIT(""), STR_LIT("Zoom Tool")},
};

void
toolbar_component_render(s32                    *active_tool_index,
                         f32                     emphasis,
                         const UI_Theme_Palette &palette)
{
    constexpr s32 tool_count = (s32)ARRAY_COUNT(TOOLBAR_ITEMS);
    emphasis                 = CLAMP(emphasis, 0.0f, 1.0f);

    ENSURE(active_tool_index);

    ui::icon_selector_overrides selector_overrides = {};
    selector_overrides.icon_gap                    = TOOLBAR_SLOT_GAP;
    selector_overrides.horizontal_padding          = TOOLBAR_CONTAINER_PAD;
    selector_overrides.vertical_padding            = TOOLBAR_CONTAINER_PAD;
    selector_overrides.container_bg_alpha          = 0.45f + (0.18f * emphasis);
    selector_overrides.hover_overlay_alpha         = 0.32f + (0.12f * emphasis);
    selector_overrides.show_active_label           = 0;
    selector_overrides.allow_hovered_when_blocked_by_popup = 1;
    selector_overrides.tooltip_border_alpha                = 0.82f;

    ui::icon_selector("toolbar_mode_like_selector",
                      TOOLBAR_ITEMS,
                      tool_count,
                      active_tool_index,
                      TOOLBAR_CONTAINER_HEIGHT,
                      palette,
                      &selector_overrides);
}
