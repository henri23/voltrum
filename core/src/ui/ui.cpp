#include "ui.hpp"
#include "icons.hpp"
#include "ui_titlebar.hpp"

#include "core/logger.hpp"
#include "core/thread_context.hpp"
#include "platform/platform.hpp"
#include "systems/resource_system.hpp"

#include <SDL3/SDL.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <imgui_internal.h>

internal_var UI_State *state_ptr;

static const String DOCKSPACE_WINDOW_NAME    = STR_LIT("DockSpace");
static const String MAIN_DOCKSPACE_ID        = STR_LIT("MainDockspace");
static const String APP_WINDOW_SETTINGS_NAME = STR_LIT("AppWindow");

INTERNAL_FUNC b8
parse_s32(String s, s32 *out_value)
{
    if (!out_value || !s.buff || s.size == 0)
    {
        return false;
    }

    s = string_trim_whitespace(s);
    if (s.size == 0)
    {
        return false;
    }

    b8  is_negative = false;
    u64 i           = 0;

    if (s.buff[0] == '-' || s.buff[0] == '+')
    {
        is_negative = (s.buff[0] == '-');
        i           = 1;
    }

    if (i >= s.size)
    {
        return false;
    }

    s64 value = 0;
    for (; i < s.size; ++i)
    {
        const char c = s.buff[i];
        if (c < '0' || c > '9')
        {
            return false;
        }

        value = (value * 10) + (s64)(c - '0');
        if (!is_negative && value > 2147483647LL)
        {
            return false;
        }

        if (is_negative && value > 2147483648LL)
        {
            return false;
        }
    }

    *out_value = is_negative ? (s32)(-value) : (s32)value;
    return true;
}

INTERNAL_FUNC void
ui_dockspace_render(UI_State *state)
{
    UI_Dockspace_State *dockspace = &state->dockspace;

    dockspace->window_began = false;

    if (dockspace->dockspace_id == 0)
    {
        dockspace->dockspace_id =
            ImGui::GetID((const char *)MAIN_DOCKSPACE_ID.buff);
        CORE_DEBUG("Generated dockspace ID: %u", dockspace->dockspace_id);
    }

    const ImGuiViewport *viewport  = ImGui::GetMainViewport();
    ImVec2               work_pos  = viewport->WorkPos;
    ImVec2               work_size = viewport->WorkSize;

    work_pos.y += TITLEBAR_HEIGHT;
    work_size.y -= TITLEBAR_HEIGHT;

    ImGui::SetNextWindowPos(work_pos);
    ImGui::SetNextWindowSize(work_size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

    ImGui::Begin((const char *)DOCKSPACE_WINDOW_NAME.buff,
                 &dockspace->dockspace_open,
                 window_flags);
    dockspace->window_began = true;

    ImGui::PopStyleVar(3);

    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiStyle &style       = ImGui::GetStyle();
        float       minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x   = 300.0f;

        ImGui::DockSpace(dockspace->dockspace_id,
                         ImVec2(0.0f, 0.0f),
                         ImGuiDockNodeFlags_NoWindowMenuButton);

        style.WindowMinSize.x = minWinSizeX;
    }
    else
    {
        CORE_ERROR("ImGui docking is not enabled!");
    }

    if (dockspace->window_began)
    {
        ImGui::End();
        dockspace->window_began = false;
    }
}

INTERNAL_FUNC b8
load_default_fonts(UI_State *state)
{
    ImGuiIO &io    = ImGui::GetIO();
    f32      scale = state->platform->main_scale;

    // Scratch arena keeps font data alive until Build() copies it
    Scratch_Arena scratch = scratch_begin(nullptr, 0);

    static const String style_names[] = {STR_LIT("normal"),
                                         STR_LIT("italic"),
                                         STR_LIT("bold_normal"),
                                         STR_LIT("bold_italic")};
    static const String icon_font_resource_path =
        STR_LIT("fontawesome/fontawesome_normal");

    Resource font_resources[FONT_MAX_COUNT] = {};
    Resource icon_resource                  = {};
    b8       icon_loaded                    = false;

    // Load icon font once (will be merged with each text font)
    if (resource_system_load(scratch.arena,
                             (const char *)icon_font_resource_path.buff,
                             Resource_Type::FONT,
                             &icon_resource))
    {
        icon_loaded = true;
        CORE_DEBUG("Loaded FontAwesome icon font");
    }
    else
    {
        CORE_WARN("Failed to load FontAwesome icon font");
    }

    for (u8 s = 0; s < FONT_MAX_COUNT; ++s)
    {
        String font_resource_path = string_fmt(scratch.arena,
                                            "jetbrains/jetbrains_%s",
                                            (const char *)style_names[s].buff);
        const char *font_resource_path_cstr =
            font_resource_path.buff ? (const char *)font_resource_path.buff : "";

        if (!resource_system_load(scratch.arena,
                                  font_resource_path_cstr,
                                  Resource_Type::FONT,
                                  &font_resources[s]))
        {
            CORE_ERROR("Failed to load font: %.*s", (s32)(font_resource_path).size, (font_resource_path).buff ? (const char *)(font_resource_path).buff : "");
            state->fonts[s] = nullptr;
            continue;
        }

        ImFontConfig config         = {};
        config.FontDataOwnedByAtlas = false;

        f32 font_size = 20.0f * scale;
        state->fonts[s] =
            io.Fonts->AddFontFromMemoryTTF(font_resources[s].data,
                                           font_resources[s].data_size,
                                           font_size,
                                           &config);

        if (state->fonts[s])
        {
            CORE_DEBUG("Loaded font: %.*s at %.0fpt (scale=%.2f)",
                       (s32)(font_resource_path).size, (font_resource_path).buff ? (const char *)(font_resource_path).buff : "",
                       font_size,
                       scale);
        }

        // Merge icon font with this text font style
        if (icon_loaded)
        {
            ImFontConfig icon_config         = {};
            icon_config.MergeMode            = true;
            icon_config.PixelSnapH           = true;
            icon_config.FontDataOwnedByAtlas = false;
            icon_config.GlyphMinAdvanceX     = 20.0f * scale;

            constexpr ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};

            io.Fonts->AddFontFromMemoryTTF(icon_resource.data,
                                           icon_resource.data_size,
                                           18.0f * scale,
                                           &icon_config,
                                           icon_ranges);
        }
    }

    // Build font atlas - data must remain valid until this point
    if (!io.Fonts->Build())
    {
        CORE_ERROR("Failed to build font atlas");
        scratch_end(scratch);
        return false;
    }

    // All font data freed implicitly when scratch ends
    scratch_end(scratch);

    // Compensate for scaled font size so layout uses logical coordinates
    io.FontGlobalScale = (1.0f / scale) * UI_PLATFORM_SCALE;

    io.FontDefault = state->fonts[(u8)Font_Style::NORMAL];
    CORE_DEBUG("Font atlas built successfully with icon support (scale=%.2f)",
               scale);

    return true;
}

UI_State *
ui_init(Arena                        *allocator,
        Dynamic_Array<UI_Layer>      *layers,
        UI_Theme                      theme,
        PFN_titlebar_content_callback titlebar_content_callback,
        String                        logo_asset_name,
        Platform_State               *plat_state,
        void                         *global_client_state)
{
    UI_State *state = push_struct(allocator, UI_State);

    state->current_theme = theme;
    ui_themes_copy_palette(theme, &state->active_palette);
    state->titlebar_content_callback = titlebar_content_callback;
    state->logo_asset_name           = logo_asset_name;
    state->is_initialized            = true;
    state->platform                  = plat_state;
    state->layers                    = layers;
    state->global_client_state       = global_client_state;

    load_default_fonts(state);

    ui_themes_apply_palette(&state->active_palette, &ImGui::GetStyle());

    ui_titlebar_setup(state, logo_asset_name);

    // Register settings handler to persist OS window size in imgui.ini
    ImGuiSettingsHandler wh;
    wh.TypeName   = (const char *)APP_WINDOW_SETTINGS_NAME.buff;
    wh.TypeHash   = ImHashStr((const char *)APP_WINDOW_SETTINGS_NAME.buff);
    wh.UserData   = (void *)plat_state->window;
    wh.ReadOpenFn = [](ImGuiContext *,
                       ImGuiSettingsHandler *,
                       const char *) -> void * { return (void *)1; };
    wh.ReadLineFn = [](ImGuiContext *,
                       ImGuiSettingsHandler *handler,
                       void *,
                       const char *line)
    {
        SDL_Window *win = (SDL_Window *)handler->UserData;

        char line_buffer[512] = {};
        string_set(line_buffer, line);
        String line_str = string_capped(line_buffer, line_buffer + sizeof(line_buffer));

        static const String SIZE_PREFIX = STR_LIT("Size=");
        static const String POS_PREFIX  = STR_LIT("Pos=");

        s32 a = 0;
        s32 b = 0;

        if (line_str.size >= SIZE_PREFIX.size &&
            string_match(string_prefix(line_str, SIZE_PREFIX.size), SIZE_PREFIX))
        {
            String values      = string_skip(line_str, SIZE_PREFIX.size);
            u64    comma_index = string_index_of(values, ',');
            if (comma_index != (u64)-1)
            {
                String first_value  = string_prefix(values, comma_index);
                String second_value = string_skip(values, comma_index + 1);

                if (parse_s32(first_value, &a) && parse_s32(second_value, &b))
                {
                    SDL_SetWindowSize(win, a, b);
                }
            }
        }
        else if (line_str.size >= POS_PREFIX.size &&
                 string_match(string_prefix(line_str, POS_PREFIX.size), POS_PREFIX))
        {
            String values      = string_skip(line_str, POS_PREFIX.size);
            u64    comma_index = string_index_of(values, ',');
            if (comma_index != (u64)-1)
            {
                String first_value  = string_prefix(values, comma_index);
                String second_value = string_skip(values, comma_index + 1);

                if (parse_s32(first_value, &a) && parse_s32(second_value, &b))
                {
                    SDL_SetWindowPosition(win, a, b);
                }
            }
        }
    };
    wh.WriteAllFn =
        [](ImGuiContext *, ImGuiSettingsHandler *handler, ImGuiTextBuffer *buf)
    {
        SDL_Window *win = (SDL_Window *)handler->UserData;
        int         w, h, x, y;
        SDL_GetWindowSize(win, &w, &h);
        SDL_GetWindowPosition(win, &x, &y);

        Scratch_Arena scratch  = scratch_begin(nullptr, 0);
        String        settings = string_fmt(scratch.arena,
                                  "[AppWindow][Main]\nSize=%d,%d\nPos=%d,%d\n\n",
                                  w,
                                  h,
                                  x,
                                  y);

        if (settings.buff && settings.size > 0)
        {
            buf->append((const char *)settings.buff,
                        (const char *)settings.buff + settings.size);
        }

        scratch_end(scratch);
    };
    ImGui::GetCurrentContext()->SettingsHandlers.push_back(wh);

    for (UI_Layer &layer : *layers)
    {
        if (layer.on_attach)
            layer.on_attach(layer.state);
    }

    state_ptr = state;

    return state;
}

void
ui_shutdown_layers(UI_State *state)
{
    for (auto &layer : *state->layers)
    {
        if (layer.on_detach)
            layer.on_detach(layer.state);
    }

    if (state_ptr == state)
    {
        state_ptr = nullptr;
    }
}

void
ui_update_layers(UI_State *state, Frame_Context *ctx)
{
    ENSURE(state->global_client_state);

    for (auto &layer : *state->layers)
    {
        if (layer.on_update)
            layer.on_update(layer.state, state->global_client_state, ctx);
    }
}

ImDrawData *
ui_draw_layers(UI_State *state, Frame_Context *ctx)
{
    ENSURE(state->global_client_state);

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ui_titlebar_draw(state);
    ui_dockspace_render(state);

    for (auto &layer : *state->layers)
    {
        if (layer.on_render)
            layer.on_render(layer.state, state->global_client_state, ctx);
    }

    ImGui::Render();

    return ImGui::GetDrawData();
}

void
ui_set_theme_state(const UI_Theme *theme, const UI_Theme_Palette *palette)
{
    if (!state_ptr)
    {
        return;
    }

    if (!theme && !palette)
    {
        return;
    }

    if (theme)
    {
        state_ptr->current_theme = *theme;

        if (!palette)
        {
            ui_themes_copy_palette(*theme, &state_ptr->active_palette);
        }
    }

    if (palette)
    {
        state_ptr->active_palette = *palette;
    }

    ui_themes_apply_palette(&state_ptr->active_palette, &ImGui::GetStyle());
}

void
ui_get_theme_state(UI_Theme *out_theme, UI_Theme_Palette *out_palette)
{
    if (!out_theme && !out_palette)
    {
        return;
    }

    if (!state_ptr)
    {
        if (out_theme)
        {
            *out_theme = UI_Theme::DARK;
        }

        if (out_palette)
        {
            ui_themes_copy_palette(UI_Theme::DARK, out_palette);
        }

        return;
    }

    if (out_theme)
    {
        *out_theme = state_ptr->current_theme;
    }

    if (out_palette)
    {
        *out_palette = state_ptr->active_palette;
    }
}

void
ui_set_theme(UI_Theme theme)
{
    ui_set_theme_state(&theme, nullptr);
}

UI_Theme
ui_get_current_theme()
{
    UI_Theme theme = UI_Theme::DARK;
    ui_get_theme_state(&theme, nullptr);
    return theme;
}

void
ui_set_theme_palette(const UI_Theme_Palette *palette)
{
    if (!palette)
    {
        return;
    }

    ui_set_theme_state(nullptr, palette);
}

void
ui_get_theme_palette(UI_Theme_Palette *out_palette)
{
    if (!out_palette)
    {
        return;
    }

    ui_get_theme_state(nullptr, out_palette);
}
