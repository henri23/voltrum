#include "theme_selector_component.hpp"

#include "core/thread_context.hpp"
#include "global_client_state.hpp"

#include <ui/ui_widgets.hpp>

INTERNAL_FUNC b8
theme_matches_filter(UI_Theme theme, String filter)
{
    if (filter.size == 0)
    {
        return true;
    }

    String            name = ui_themes_get_name(theme);
    UI_Theme_Metadata meta = ui_themes_get_metadata(theme);

    return string_find(name, 0, filter, String_Match_Flags::CASE_INSENSITIVE) !=
               (u64)-1 ||
           string_find(meta.description,
                       0,
                       filter,
                       String_Match_Flags::CASE_INSENSITIVE) != (u64)-1 ||
           string_find(meta.keywords,
                       0,
                       filter,
                       String_Match_Flags::CASE_INSENSITIVE) != (u64)-1;
}

INTERNAL_FUNC void
queue_theme_change(Global_Client_State *global_state, UI_Theme theme)
{
    if (!global_state)
    {
        return;
    }

    global_state->requested_theme      = theme;
    global_state->request_theme_change = true;
}

void
theme_selector_component_on_open(void                *component_state,
                                 Global_Client_State *global_state)
{
    (void)global_state;

    auto *state = (Theme_Selector_Component_State *)component_state;
    if (!state)
    {
        return;
    }

    state->selection = 0;
}

void
theme_selector_component_render(void                   *component_state,
                                Global_Client_State    *global_state,
                                const UI_Theme_Palette &palette,
                                String                  filter,
                                b8                     *request_close)
{
    (void)request_close;

    auto *state = (Theme_Selector_Component_State *)component_state;
    if (!state || !global_state)
    {
        return;
    }

    ui::accent_row_style row_style = ui::make_accent_row_style(palette);

    constexpr s32 theme_count                        = (s32)UI_Theme::MAX_COUNT;
    s32           visible_theme_indices[theme_count] = {};
    s32           visible_count                      = 0;

    for (s32 i = 0; i < theme_count; ++i)
    {
        UI_Theme theme = (UI_Theme)i;
        if (!theme_matches_filter(theme, filter))
        {
            continue;
        }

        visible_theme_indices[visible_count++] = i;
    }

    if (visible_count == 0)
    {
        ImGui::TextUnformatted("No themes matched your filter.");
        return;
    }

    if (state->selection < 0 || state->selection >= visible_count)
    {
        state->selection = 0;
    }

    b8 moved_by_keyboard = false;
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, false))
    {
        state->selection  = (state->selection + 1) % visible_count;
        moved_by_keyboard = true;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, false))
    {
        state->selection -= 1;
        if (state->selection < 0)
        {
            state->selection = visible_count - 1;
        }
        moved_by_keyboard = true;
    }

    Scratch_Arena scratch = scratch_begin(nullptr, 0);

    for (s32 visible_index = 0; visible_index < visible_count; ++visible_index)
    {
        UI_Theme theme         = (UI_Theme)visible_theme_indices[visible_index];
        String   theme_name    = ui_themes_get_name(theme);
        UI_Theme_Metadata meta = ui_themes_get_metadata(theme);
        const b8          is_selected = visible_index == state->selection;

        String description = meta.description;
        if (theme == global_state->target_theme)
        {
            if (global_state->is_theme_transitioning)
            {
                description = string_fmt(
                    scratch.arena,
                    "%.*s  -  Applying...",
                    (s32)meta.description.size,
                    meta.description.buff ? (const char *)meta.description.buff
                                          : "");
            }
            else
            {
                description = string_fmt(
                    scratch.arena,
                    "%.*s  -  Current",
                    (s32)meta.description.size,
                    meta.description.buff ? (const char *)meta.description.buff
                                          : "");
            }
        }

        String row_id =
            string_fmt(scratch.arena, "##theme_row_%d", visible_index);
        if (ui::accent_row(row_id.buff ? (const char *)row_id.buff : "",
                           theme_name.buff ? (const char *)theme_name.buff : "",
                           description.buff ? (const char *)description.buff
                                            : "",
                           row_style,
                           is_selected,
                           56.0f))
        {
            queue_theme_change(global_state, theme);
        }

        if (is_selected && moved_by_keyboard)
        {
            ImGui::SetScrollHereY(0.35f);
        }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) && state->selection >= 0 &&
        state->selection < visible_count)
    {
        UI_Theme selected_theme =
            (UI_Theme)visible_theme_indices[state->selection];
        queue_theme_change(global_state, selected_theme);
    }

    scratch_end(scratch);
}
