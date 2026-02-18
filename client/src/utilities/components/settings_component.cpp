#include "settings_component.hpp"

#include "global_client_state.hpp"

#include <imgui.h>

INTERNAL_FUNC b8
filter_matches_label(String filter, String label)
{
    if (filter.size == 0)
    {
        return true;
    }

    return string_find(label,
                           0,
                           filter,
                           String_Match_Flags::CASE_INSENSITIVE) != (u64)-1;
}

void
settings_component_render(void                   *component_state,
                          Global_Client_State    *global_state,
                          const UI_Theme_Palette &palette,
                          String                  filter,
                          b8                     *request_close)
{
    (void)component_state;
    (void)palette;

    if (!global_state)
    {
        return;
    }

    const b8 show_demo_windows =
        filter_matches_label(filter, STR_LIT("imgui demo")) ||
        filter_matches_label(filter, STR_LIT("implot demo")) ||
        filter_matches_label(filter, STR_LIT("developer windows"));

    const b8 show_palette_controls =
        filter_matches_label(filter, STR_LIT("palette")) ||
        filter_matches_label(filter, STR_LIT("close"));

    if (!show_demo_windows && !show_palette_controls)
    {
        ImGui::TextUnformatted("No settings matched your filter.");
        return;
    }

    if (show_demo_windows)
    {
        ImGui::TextUnformatted("Developer Windows");
        ImGui::Checkbox("ImGui Demo", &global_state->is_imgui_demo_visible);
        ImGui::Checkbox("ImPlot Demo", &global_state->is_implot_demo_visible);
#ifdef DEBUG_BUILD
        ImGui::Checkbox("Memory Inspector", &global_state->is_debug_layer_visible);
#endif
    }

    if (show_palette_controls)
    {
        if (show_demo_windows)
        {
            ImGui::Separator();
        }

        ImGui::TextUnformatted("Palette");
        if (ImGui::Button("Close Command Palette") && request_close)
        {
            *request_close = true;
        }
    }
}
