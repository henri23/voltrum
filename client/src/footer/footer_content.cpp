#include "footer_content.hpp"

#include <core/thread_context.hpp>
#include <imgui.h>
#include <utils/string.hpp>

void
client_footer_content_callback(void                   *client_state,
                               vec2                    content_area_min,
                               vec2                    content_area_max,
                               const UI_Theme_Palette *palette)
{
    (void)client_state;
    if (!palette)
    {
        return;
    }

    const char *license_tier = "Free";

#if defined(DEBUG_BUILD)
    const char *build_mode = "Debug";
#elif defined(RELEASE_BUILD)
    const char *build_mode = "Release";
#else
    const char *build_mode = "Unknown";
#endif

    Scratch_Arena scratch = scratch_begin(nullptr, 0);
    String left_text = string_fmt(
        scratch.arena, "License: %s  |  Build: %s", license_tier, build_mode);

    const f32 text_height = ImGui::GetTextLineHeight();
    const f32 text_y =
        content_area_min.y + (content_area_max.y - content_area_min.y - text_height) * 0.5f;

    ImGui::SetCursorScreenPos(ImVec2(content_area_min.x, text_y));
    ImGui::PushStyleColor(ImGuiCol_Text,
                          ImGui::ColorConvertU32ToFloat4(palette->text));
    const char *left_begin = left_text.buff ? (const char *)left_text.buff : "";
    const char *left_end   = left_begin + left_text.size;
    ImGui::TextUnformatted(left_begin, left_end);
    ImGui::PopStyleColor();

    const f32 fps = ImGui::GetIO().Framerate;
    const f32 frame_ms = (fps > 0.0f) ? (1000.0f / fps) : 0.0f;

    String right_text = string_fmt(scratch.arena,
                                   "Ping: -- ms  |  FPS: %.0f  |  Frame: %.2f ms",
                                   fps,
                                   frame_ms);

    const char *right_begin = right_text.buff ? (const char *)right_text.buff : "";
    const char *right_end   = right_begin + right_text.size;
    ImVec2 right_text_size  = ImGui::CalcTextSize(right_begin, right_end);
    f32 right_x = content_area_max.x - right_text_size.x;
    if (right_x < content_area_min.x)
    {
        right_x = content_area_min.x;
    }

    ImGui::SetCursorScreenPos(ImVec2(right_x, text_y));
    ImGui::PushStyleColor(ImGuiCol_Text,
                          ImGui::ColorConvertU32ToFloat4(palette->text_darker));
    ImGui::TextUnformatted(right_begin, right_end);
    ImGui::PopStyleColor();

    scratch_end(scratch);
}
