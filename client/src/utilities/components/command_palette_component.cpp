#include "command_palette_component.hpp"

#include "core/thread_context.hpp"
#include "global_client_state.hpp"

#include <ui/icons.hpp>
#include <ui/ui_widgets.hpp>

static const String COMMAND_PALETTE_WINDOW_ID =
    STR_LIT("##UtilitiesCommandPalette");
static const String COMMAND_PALETTE_FILTER_ID =
    STR_LIT("##command_palette_filter");
static const String ROOT_SECTION_LABEL = STR_LIT("Components");
static const String PALETTE_TITLE_TEXT = STR_LIT("Command Palette");

void
command_palette_init(Command_Palette_State           *state,
                     const Command_Palette_Component *components,
                     s32                              component_count)
{
    if (!state)
    {
        return;
    }

    *state                        = {};
    state->active_component_index = -1;
    state->focus_filter           = true;
    state->command_filter[0]      = '\0';

    if (!components || component_count <= 0)
    {
        return;
    }

    const s32 capped_count =
        MIN(component_count, COMMAND_PALETTE_MAX_COMPONENT_COUNT);
    for (s32 i = 0; i < capped_count; ++i)
    {
        const Command_Palette_Component &component = components[i];
        if (component.label.size == 0 || !component.on_render)
        {
            continue;
        }

        state->components[state->component_count++] = component;
    }
}

void
command_palette_component_render(Command_Palette_State  *state,
                                 Global_Client_State    *global_state,
                                 const UI_Theme_Palette &palette,
                                 f32                     delta_time)
{
    if (!state || !global_state)
    {
        return;
    }

    if (state->active_component_index < -1 ||
        state->active_component_index >= state->component_count)
    {
        state->active_component_index = -1;
    }

    ImGuiIO &io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_K, false))
    {
        if (global_state->is_command_palette_open)
        {
            global_state->request_close_command_palette = true;
            global_state->request_open_command_palette  = false;
        }
        else
        {
            global_state->request_open_command_palette  = true;
            global_state->request_close_command_palette = false;
        }
    }

    b8 open_requested = false;
    if (global_state->request_open_command_palette)
    {
        global_state->request_open_command_palette  = false;
        global_state->request_close_command_palette = false;
        global_state->is_command_palette_open       = true;

        state->intro_t                = 0.0f;
        state->selection              = 0;
        state->active_component_index = -1;
        state->focus_filter           = true;
        state->command_filter[0]      = '\0';

        open_requested = true;
    }

    b8 close_requested = false;
    if (global_state->request_close_command_palette)
    {
        close_requested                             = true;
        global_state->request_close_command_palette = false;
        global_state->request_open_command_palette  = false;
        global_state->is_command_palette_open       = false;
    }

    const b8 popup_is_open =
        ImGui::IsPopupOpen((const char *)COMMAND_PALETTE_WINDOW_ID.buff,
                           ImGuiPopupFlags_None);
    if (!global_state->is_command_palette_open && !popup_is_open &&
        !open_requested)
    {
        state->selection = 0;
        return;
    }

    if (open_requested)
    {
        ImGui::OpenPopup((const char *)COMMAND_PALETTE_WINDOW_ID.buff);
    }

    Scratch_Arena scratch = scratch_begin(nullptr, 0);

    const ImGuiViewport *viewport   = ImGui::GetMainViewport();
    ImVec2               popup_size = ImVec2(740.0f, 460.0f);
    ImVec2               popup_pos =
        ImVec2(viewport->Pos.x + (viewport->Size.x - popup_size.x) * 0.5f,
               viewport->Pos.y + viewport->Size.y * 0.16f);

    state->intro_t =
        ui::anim::exp_decay_to(state->intro_t, 1.0f, 14.0f, delta_time);
    const f32 intro = CLAMP(state->intro_t, 0.0f, 1.0f);

    ImVec4 bg     = ImGui::ColorConvertU32ToFloat4(palette.window_bg);
    ImVec4 text   = ImGui::ColorConvertU32ToFloat4(palette.text);
    ImVec4 dim    = ImGui::ColorConvertU32ToFloat4(palette.text_darker);
    ImVec4 accent = ImGui::ColorConvertU32ToFloat4(palette.accent);

    ImGui::SetNextWindowPos(popup_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(popup_size, ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 16.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.6f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 12.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.40f + (0.60f * intro));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(bg.x, bg.y, bg.z, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border,
                          ImVec4(accent.x, accent.y, accent.z, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_Separator,
                          ImVec4(accent.x, accent.y, accent.z, 0.55f));

    ImGuiWindowFlags popup_flags =
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoMove;

    b8 close_now = close_requested;

    if (ImGui::BeginPopup((const char *)COMMAND_PALETTE_WINDOW_ID.buff,
                          popup_flags))
    {
        if (state->active_component_index >= 0)
        {
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImVec4(accent.x, accent.y, accent.z, 0.24f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                  ImVec4(accent.x, accent.y, accent.z, 0.44f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                                  ImVec4(accent.x, accent.y, accent.z, 0.60f));
            if (ImGui::Button(ICON_FA_ARROW_LEFT " Back"))
            {
                state->active_component_index = -1;
                state->selection              = 0;
                state->focus_filter           = true;
                state->command_filter[0]      = '\0';
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine();
        }

        ImGui::PushStyleColor(ImGuiCol_Text,
                              ImVec4(text.x, text.y, text.z, 0.99f));
        ImGui::TextUnformatted(ICON_FA_TERMINAL);
        ImGui::SameLine(0.0f, 5.0f);
        ImGui::TextUnformatted((const char *)PALETTE_TITLE_TEXT.buff);
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text,
                              ImVec4(dim.x, dim.y, dim.z, 0.95f));
        String section_label = ROOT_SECTION_LABEL;
        if (state->active_component_index >= 0 &&
            state->active_component_index < state->component_count)
        {
            section_label =
                state->components[state->active_component_index].label;
        }
        ImGui::TextUnformatted(
            section_label.buff ? (const char *)section_label.buff : "");
        ImGui::PopStyleColor();

        ImGui::Separator();

        const String filter_hint = (state->active_component_index < 0)
                                       ? STR_LIT("Search components...")
                                       : STR_LIT("Filter component...");

        ImGui::PushItemWidth(-1.0f);
        ImGui::InputTextWithHint((const char *)COMMAND_PALETTE_FILTER_ID.buff,
                                 (const char *)filter_hint.buff,
                                 state->command_filter,
                                 sizeof(state->command_filter));
        ImGui::PopItemWidth();

        if (state->focus_filter)
        {
            ImGui::SetKeyboardFocusHere(-1);
            state->focus_filter = false;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
        {
            close_now = true;
        }

        ImGui::Separator();

        if (state->active_component_index < 0)
        {
            ui::accent_row_style row_style = ui::make_accent_row_style(palette);
            String               filter    = STR(state->command_filter);
            s32 visible_indices[COMMAND_PALETTE_MAX_COMPONENT_COUNT] = {};
            s32 visible_count                                        = 0;

            for (s32 i = 0; i < state->component_count; ++i)
            {
                const Command_Palette_Component &component =
                    state->components[i];

                if (filter.size > 0 &&
                    string_find(component.label,
                                0,
                                filter,
                                String_Match_Flags::CASE_INSENSITIVE) ==
                        (u64)-1 &&
                    string_find(component.description,
                                0,
                                filter,
                                String_Match_Flags::CASE_INSENSITIVE) ==
                        (u64)-1 &&
                    string_find(component.keywords,
                                0,
                                filter,
                                String_Match_Flags::CASE_INSENSITIVE) ==
                        (u64)-1)
                {
                    continue;
                }

                visible_indices[visible_count++] = i;
            }

            b8 moved_by_keyboard = false;
            if (visible_count <= 0)
            {
                state->selection = 0;
            }
            else
            {
                if (state->selection < 0 || state->selection >= visible_count)
                {
                    state->selection = 0;
                }

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
            }

            if (visible_count == 0)
            {
                ImGui::PushStyleColor(ImGuiCol_Text,
                                      ImVec4(dim.x, dim.y, dim.z, 0.96f));
                ImGui::TextUnformatted("No components matched your search.");
                ImGui::PopStyleColor();
            }
            else
            {
                for (s32 visible_index = 0; visible_index < visible_count;
                     ++visible_index)
                {
                    const s32 component_index = visible_indices[visible_index];
                    const Command_Palette_Component &component =
                        state->components[component_index];

                    const b8 selected = visible_index == state->selection;
                    String   row_id   = string_fmt(scratch.arena,
                                               "##cp_component_%d",
                                               visible_index);

                    if (ui::accent_row(
                            row_id.buff ? (const char *)row_id.buff : "",
                            component.label.buff
                                ? (const char *)component.label.buff
                                : "",
                            component.description.buff
                                ? (const char *)component.description.buff
                                : "",
                            row_style,
                            selected,
                            58.0f))
                    {
                        state->active_component_index = component_index;
                        state->selection              = 0;
                        state->focus_filter           = true;
                        state->command_filter[0]      = '\0';
                        if (component.on_open)
                        {
                            component.on_open(component.component_state,
                                              global_state);
                        }
                        break;
                    }

                    if (selected && moved_by_keyboard)
                    {
                        ImGui::SetScrollHereY(0.35f);
                    }
                }

                if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) &&
                    state->selection >= 0 && state->selection < visible_count)
                {
                    const s32 component_index =
                        visible_indices[state->selection];
                    const Command_Palette_Component &component =
                        state->components[component_index];

                    state->active_component_index = component_index;
                    state->selection              = 0;
                    state->focus_filter           = true;
                    state->command_filter[0]      = '\0';
                    if (component.on_open)
                    {
                        component.on_open(component.component_state,
                                          global_state);
                    }
                }
            }
        }
        else
        {
            if (state->active_component_index >= 0 &&
                state->active_component_index < state->component_count)
            {
                const Command_Palette_Component &component =
                    state->components[state->active_component_index];
                b8 request_close = false;

                component.on_render(component.component_state,
                                    global_state,
                                    palette,
                                    STR(state->command_filter),
                                    &request_close);

                if (request_close)
                {
                    close_now = true;
                }
            }
            else
            {
                state->active_component_index = -1;
                state->selection              = 0;
                state->focus_filter           = true;
                state->command_filter[0]      = '\0';
            }
        }

        if (close_now)
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    const b8 popup_open_after_render =
        ImGui::IsPopupOpen((const char *)COMMAND_PALETTE_WINDOW_ID.buff,
                           ImGuiPopupFlags_None);
    global_state->is_command_palette_open       = popup_open_after_render;
    global_state->request_open_command_palette  = false;
    global_state->request_close_command_palette = false;

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(6);
    scratch_end(scratch);
}
