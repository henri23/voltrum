#include "ui_footer.hpp"

#include "ui_themes.hpp"
#include "ui_types.hpp"

#include <imgui_internal.h>

INTERNAL_FUNC u32
ui_footer_separator_color(const UI_Theme_Palette *palette)
{
    ImVec4 color = ImGui::ColorConvertU32ToFloat4(palette->separator_hovered);
    color.w      = ImClamp(color.w * 0.65f, 0.0f, 1.0f);
    return ImGui::ColorConvertFloat4ToU32(color);
}

void
ui_footer_draw(UI_State *context)
{
    UI_Footer_State        *state   = &context->footer;
    const UI_Theme_Palette *palette = &context->active_palette;

    ImGuiViewport *viewport    = ImGui::GetMainViewport();
    ImVec2         window_pos  = viewport->Pos;
    ImVec2         window_size = viewport->Size;

    state->footer_min = ImVec2(window_pos.x, window_pos.y + window_size.y - FOOTER_HEIGHT);
    state->footer_max = ImVec2(window_pos.x + window_size.x, window_pos.y + window_size.y);

    ImGui::SetNextWindowPos(state->footer_min);
    ImGui::SetNextWindowSize(ImVec2(window_size.x, FOOTER_HEIGHT));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImDrawList *bg_draw_list = ImGui::GetBackgroundDrawList(viewport);
    bg_draw_list->AddRectFilled(state->footer_min,
                                state->footer_max,
                                palette->background_dark);
    bg_draw_list->AddLine(ImVec2(state->footer_min.x, state->footer_min.y),
                          ImVec2(state->footer_max.x, state->footer_min.y),
                          ui_footer_separator_color(palette),
                          1.0f);

    if (ImGui::Begin("##CustomFooter", nullptr, flags))
    {
        ImGui::GetCurrentWindow()->DockNode = nullptr;

        f32 horizontal_padding = IM_ROUND(10.0f * UI_PLATFORM_SCALE);
        f32 vertical_padding   = IM_ROUND(4.0f * UI_PLATFORM_SCALE);

        state->content_area_min = {
            state->footer_min.x + horizontal_padding,
            state->footer_min.y + vertical_padding};
        state->content_area_max = {
            state->footer_max.x - horizontal_padding,
            state->footer_max.y - vertical_padding};

        if (context->footer_content_callback)
        {
            context->footer_content_callback(context->global_client_state,
                                             state->content_area_min,
                                             state->content_area_max,
                                             palette);
        }

        ImVec2 mouse_pos = ImGui::GetMousePos();
        state->is_footer_hovered =
            mouse_pos.x >= state->footer_min.x &&
            mouse_pos.x <= state->footer_max.x &&
            mouse_pos.y >= state->footer_min.y &&
            mouse_pos.y <= state->footer_max.y;
    }

    ImGui::End();
}
