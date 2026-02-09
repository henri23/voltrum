#include "command_palette_component.hpp"

#include "core/thread_context.hpp"
#include "global_client_state.hpp"
#include "utils/string.hpp"

#include <ui/icons.hpp>
#include <ui/ui_widgets.hpp>

static const String COMMAND_PALETTE_WINDOW_ID =
    STR_LIT("##UtilitiesCommandPalette");
static const String COMMAND_PALETTE_FILTER_ID =
    STR_LIT("##command_palette_filter");
static const String ROOT_SECTION_LABEL = STR_LIT("Sections");
static const String PALETTE_TITLE_TEXT = STR_LIT("Command Palette");

INTERNAL_FUNC void
reset_local_navigation_state(Command_Palette_State *state)
{
    state->selection         = 0;
    state->focus_filter      = true;
    state->command_filter[0] = '\0';
}

INTERNAL_FUNC void
set_active_parent(Command_Palette_State *state, String parent_id)
{
    state->active_parent_id =
        const_str_from_str<COMMAND_PALETTE_ID_MAX_LENGTH>(parent_id);
    reset_local_navigation_state(state);
}

INTERNAL_FUNC b8
ensure_state(Command_Palette_State *state)
{
    if (!state)
    {
        return false;
    }

    if (!state->initialized)
    {
        command_palette_init(state);
    }

    return true;
}

INTERNAL_FUNC f32
ease_out_cubic(f32 t)
{
    t             = CLAMP(t, 0.0f, 1.0f);
    const f32 inv = 1.0f - t;
    return 1.0f - (inv * inv * inv);
}

INTERNAL_FUNC b8
str_contains_case_insensitive(String haystack, String needle)
{
    return str_find_needle(haystack,
                           0,
                           needle,
                           String_Match_Flags::CASE_INSENSITIVE) != (u64)-1;
}

INTERNAL_FUNC b8
matches_filter(String label, String description, String keywords, String filter)
{
    if (filter.size == 0)
    {
        return true;
    }

    return str_contains_case_insensitive(label, filter) ||
           str_contains_case_insensitive(description, filter) ||
           str_contains_case_insensitive(keywords, filter);
}

INTERNAL_FUNC void
toggle_palette_open(Global_Client_State *global_state)
{
    const b8 will_open = !global_state->is_command_palette_open;
    global_state->is_command_palette_open       = will_open;
    global_state->request_open_command_palette  = will_open;
    global_state->request_close_command_palette = !will_open;
}

INTERNAL_FUNC b8
active_parent_is_root(const Command_Palette_State *state)
{
    return state->active_parent_id.size == 0;
}

INTERNAL_FUNC s32
find_command_index_by_id(const Command_Palette_State *state, String command_id)
{
    if (command_id.size == 0)
    {
        return -1;
    }

    for (s32 i = 0; i < state->command_count; ++i)
    {
        String registered_id = state->commands[i].id;
        if (str_match(registered_id, command_id))
        {
            return i;
        }
    }

    return -1;
}

INTERNAL_FUNC b8
command_has_children(const Command_Palette_State *state, String command_id)
{
    if (command_id.size == 0)
    {
        return false;
    }

    for (s32 i = 0; i < state->command_count; ++i)
    {
        String parent_id = state->commands[i].parent_id;
        if (str_match(parent_id, command_id))
        {
            return true;
        }
    }

    return false;
}

INTERNAL_FUNC b8
handle_navigation_keys(Command_Palette_State *state, s32 visible_count)
{
    b8 moved_by_keyboard = false;

    if (visible_count <= 0)
    {
        state->selection = 0;
        return false;
    }

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

    return moved_by_keyboard;
}

INTERNAL_FUNC String
current_section_label(const Command_Palette_State *state)
{
    if (active_parent_is_root(state))
    {
        return ROOT_SECTION_LABEL;
    }

    s32 parent_index = find_command_index_by_id(state, state->active_parent_id);
    if (parent_index < 0)
    {
        return ROOT_SECTION_LABEL;
    }

    return state->commands[parent_index].label;
}

INTERNAL_FUNC void
navigate_back(Command_Palette_State *state)
{
    if (active_parent_is_root(state))
    {
        return;
    }

    s32 parent_index = find_command_index_by_id(state, state->active_parent_id);
    if (parent_index < 0)
    {
        set_active_parent(state, str_zero());
        return;
    }

    String next_parent = state->commands[parent_index].parent_id;
    set_active_parent(state, next_parent);
}

INTERNAL_FUNC String
resolve_command_description(const Command_Palette_Registered_Command &command,
                            struct Arena                             *arena,
                            void                                     *global_state)
{
    String description = command.description;
    if (!command.resolve_description)
    {
        return description;
    }

    String resolved = command.resolve_description(
        arena, global_state, command.user_data, description);

    return (resolved.str && resolved.size > 0) ? resolved : description;
}

INTERNAL_FUNC void
activate_command(Command_Palette_State                    *state,
                 const Command_Palette_Registered_Command *command,
                 Global_Client_State                      *global_state,
                 b8                                       *keep_open)
{
    if (!command)
    {
        return;
    }

    String command_id = command->id;
    if (command_has_children(state, command_id))
    {
        set_active_parent(state, command_id);
        return;
    }

    if (command->on_execute)
    {
        command->on_execute(global_state, command->user_data);
    }

    if (command->close_on_execute && keep_open)
    {
        *keep_open = false;
    }
}

void
command_palette_init(Command_Palette_State *state)
{
    if (!state)
    {
        return;
    }

    *state              = {};
    state->initialized  = true;
    state->focus_filter = true;
}

void
command_palette_reset_state(Command_Palette_State *state)
{
    if (!ensure_state(state))
    {
        return;
    }

    state->intro_t = 0.0f;
    set_active_parent(state, str_zero());
}

void
command_palette_clear_registry(Command_Palette_State *state)
{
    if (!ensure_state(state))
    {
        return;
    }

    state->command_count = 0;
    set_active_parent(state, str_zero());
}

b8
command_palette_register(Command_Palette_State                    *state,
                         const Command_Palette_Command_Definition *command)
{
    if (!ensure_state(state))
    {
        return false;
    }

    if (!command || command->id.size == 0 || command->label.size == 0)
    {
        return false;
    }

    if (command->parent_id.size > 0 &&
        str_match(command->id, command->parent_id))
    {
        return false;
    }

    s32 command_index = find_command_index_by_id(state, command->id);
    if (command_index < 0)
    {
        if (state->command_count >= COMMAND_PALETTE_MAX_COMMAND_COUNT)
        {
            return false;
        }

        command_index = state->command_count++;
    }

    Command_Palette_Registered_Command &stored = state->commands[command_index];
    stored.id = const_str_from_str<COMMAND_PALETTE_ID_MAX_LENGTH>(command->id);
    stored.parent_id =
        const_str_from_str<COMMAND_PALETTE_ID_MAX_LENGTH>(command->parent_id);
    stored.label =
        const_str_from_str<COMMAND_PALETTE_LABEL_MAX_LENGTH>(command->label);
    stored.description =
        const_str_from_str<COMMAND_PALETTE_DESCRIPTION_MAX_LENGTH>(
            command->description);
    stored.keywords = const_str_from_str<COMMAND_PALETTE_KEYWORDS_MAX_LENGTH>(
        command->keywords);
    stored.on_execute          = command->on_execute;
    stored.resolve_description = command->resolve_description;
    stored.user_data           = command->user_data;
    stored.close_on_execute    = command->close_on_execute;

    return true;
}

void
command_palette_component_render(Command_Palette_State  *state,
                                 Global_Client_State    *global_state,
                                 const UI_Theme_Palette &palette,
                                 f32                     delta_time)
{
    if (!ensure_state(state) || !global_state)
    {
        return;
    }

    if (!active_parent_is_root(state) &&
        find_command_index_by_id(state, state->active_parent_id) < 0)
    {
        set_active_parent(state, str_zero());
    }

    ImGuiIO &io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_K, false))
    {
        toggle_palette_open(global_state);
    }

    if (global_state->request_open_command_palette)
    {
        global_state->request_open_command_palette  = false;
        global_state->request_close_command_palette = false;
        global_state->is_command_palette_open       = true;
        command_palette_reset_state(state);
    }

    if (global_state->request_close_command_palette)
    {
        global_state->is_command_palette_open       = false;
        global_state->request_close_command_palette = false;
    }

    if (!global_state->is_command_palette_open)
    {
        state->selection = 0;
        return;
    }

    Scratch_Arena scratch = scratch_begin(nullptr, 0);

    const ImGuiViewport *viewport   = ImGui::GetMainViewport();
    ImVec2               popup_size = ImVec2(740.0f, 460.0f);
    ImVec2               popup_pos =
        ImVec2(viewport->Pos.x + (viewport->Size.x - popup_size.x) * 0.5f,
               viewport->Pos.y + viewport->Size.y * 0.16f);

    state->intro_t += CLAMP(delta_time * 5.2f, 0.0f, 1.0f);
    const f32 intro = ease_out_cubic(state->intro_t);

    ImVec4 border = ImGui::ColorConvertU32ToFloat4(palette.selection);
    ImVec4 bg     = ImGui::ColorConvertU32ToFloat4(palette.window_bg);
    ImVec4 text   = ImGui::ColorConvertU32ToFloat4(palette.text);
    ImVec4 dim    = ImGui::ColorConvertU32ToFloat4(palette.text_darker);
    ImVec4 accent = ImGui::ColorConvertU32ToFloat4(palette.accent);

    ImGui::SetNextWindowPos(popup_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(popup_size, ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 12.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(bg.x, bg.y, bg.z, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(border.x, border.y, border.z, 0.0f));

    b8 keep_open = global_state->is_command_palette_open == true;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking |
                                    ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoScrollbar |
                                    ImGuiWindowFlags_NoSavedSettings |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoBackground;
    if (ImGui::Begin(C_STR(COMMAND_PALETTE_WINDOW_ID), &keep_open, window_flags))
    {
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        ImVec2 window_min = ImGui::GetWindowPos();
        ImVec2 window_max = ImVec2(window_min.x + ImGui::GetWindowSize().x,
                                   window_min.y + ImGui::GetWindowSize().y);
        ui::draw_glass_container(draw_list,
                                 window_min,
                                 window_max,
                                 palette,
                                 0.84f + (0.16f * intro),
                                 16.0f);

        ImGui::PushStyleColor(ImGuiCol_Text,
                              ImVec4(text.x, text.y, text.z, 0.99f));

        if (!active_parent_is_root(state))
        {
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImVec4(accent.x, accent.y, accent.z, 0.24f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                  ImVec4(accent.x, accent.y, accent.z, 0.44f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                                  ImVec4(accent.x, accent.y, accent.z, 0.60f));
            if (ImGui::Button(ICON_FA_ARROW_LEFT " Back"))
            {
                navigate_back(state);
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine();
        }

        ImGui::TextUnformatted(ICON_FA_TERMINAL);
        ImGui::SameLine(0.0f, 5.0f);
        ImGui::TextUnformatted(C_STR(PALETTE_TITLE_TEXT));
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text,
                              ImVec4(dim.x, dim.y, dim.z, 0.95f));
        ImGui::TextUnformatted(C_STR(current_section_label(state)));
        ImGui::PopStyleColor();

        ImGui::Separator();

        const String filter_hint = active_parent_is_root(state)
                                       ? STR_LIT("Search commands...")
                                       : STR_LIT("Search section...");

        ImGui::PushItemWidth(-1.0f);
        ImGui::InputTextWithHint(C_STR(COMMAND_PALETTE_FILTER_ID),
                                 C_STR(filter_hint),
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
            if (active_parent_is_root(state))
            {
                keep_open = false;
            }
            else
            {
                navigate_back(state);
            }
        }

        const b8 hovered_window = ImGui::IsWindowHovered(
            ImGuiHoveredFlags_AllowWhenBlockedByPopup |
            ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left, false) && !hovered_window)
        {
            keep_open = false;
        }

        ImGui::Separator();

        ui::accent_row_style row_style = ui::make_accent_row_style(palette);
        String             filter    = STR(state->command_filter);
        s32 visible_indices[COMMAND_PALETTE_MAX_COMMAND_COUNT] = {};
        s32 visible_count                                      = 0;

        for (s32 i = 0; i < state->command_count; ++i)
        {
            const Command_Palette_Registered_Command &command =
                state->commands[i];
            String parent_id = command.parent_id;

            const b8 is_current_parent =
                active_parent_is_root(state)
                    ? (parent_id.size == 0)
                    : str_match(parent_id, state->active_parent_id);

            if (!is_current_parent)
            {
                continue;
            }

            if (!matches_filter(command.label,
                                command.description,
                                command.keywords,
                                filter))
            {
                continue;
            }

            visible_indices[visible_count++] = i;
        }

        const b8 keyboard_nav = handle_navigation_keys(state, visible_count);

        if (visible_count == 0)
        {
            ImGui::PushStyleColor(ImGuiCol_Text,
                                  ImVec4(dim.x, dim.y, dim.z, 0.96f));
            ImGui::TextUnformatted("No commands matched your search.");
            ImGui::PopStyleColor();
        }
        else
        {
            for (s32 visible_index = 0; visible_index < visible_count;
                 ++visible_index)
            {
                s32 command_index = visible_indices[visible_index];
                const Command_Palette_Registered_Command &command =
                    state->commands[command_index];
                const b8 selected = visible_index == state->selection;

                String label       = command.label;
                String description = resolve_command_description(command,
                                                                 scratch.arena,
                                                                 global_state);

                String row_id =
                    str_fmt(scratch.arena, "##cp_command_%d", visible_index);
                if (ui::accent_row(C_STR(row_id),
                                  C_STR(label),
                                  C_STR(description),
                                  row_style,
                                  selected,
                                  58.0f))
                {
                    activate_command(state, &command, global_state, &keep_open);
                }

                if (selected && keyboard_nav)
                {
                    ImGui::SetScrollHereY(0.35f);
                }
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Enter, false))
            {
                const Command_Palette_Registered_Command &command =
                    state->commands[visible_indices[state->selection]];
                activate_command(state, &command, global_state, &keep_open);
            }
        }

    }
    ImGui::End();

    global_state->is_command_palette_open       = keep_open;
    global_state->request_open_command_palette  = false;
    global_state->request_close_command_palette = false;

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(4);
    scratch_end(scratch);
}
