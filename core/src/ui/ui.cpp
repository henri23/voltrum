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

INTERNAL_FUNC void
ui_dockspace_render(UI_State *state)
{
    UI_Dockspace_State *dockspace = &state->dockspace;

    dockspace->window_began = false;

    if (dockspace->dockspace_id == 0)
    {
        dockspace->dockspace_id = ImGui::GetID("MainDockspace");
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

    const char *window_name = "DockSpace";
    ImGui::Begin(window_name, &dockspace->dockspace_open, window_flags);
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
    ImGuiIO &io = ImGui::GetIO();
    f32 scale = state->platform->main_scale;

    // Scratch arena keeps font data alive until Build() copies it
    Scratch_Arena scratch = scratch_begin(nullptr, 0);

    constexpr const char *style_names[] = {"normal",
                                           "italic",
                                           "bold_normal",
                                           "bold_italic"};

    Resource font_resources[FONT_MAX_COUNT] = {};
    Resource icon_resource                  = {};
    b8       icon_loaded                    = false;

    // Load icon font once (will be merged with each text font)
    if (resource_system_load(scratch.arena,
                             "fontawesome/fontawesome_normal",
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
        char path[128];
        u32  i = 0;

        const char *prefix = "jetbrains/jetbrains_";
        for (u32 j = 0; prefix[j] != '\0'; ++j)
            path[i++] = prefix[j];

        for (u32 j = 0; style_names[s][j] != '\0'; ++j)
            path[i++] = style_names[s][j];

        path[i] = '\0';

        if (!resource_system_load(scratch.arena,
                                  path,
                                  Resource_Type::FONT,
                                  &font_resources[s]))
        {
            CORE_ERROR("Failed to load font: %s", path);
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
            CORE_DEBUG("Loaded font: %s at %.0fpt (scale=%.2f)", path, font_size, scale);
        }

        // Merge icon font with this text font style
        if (icon_loaded)
        {
            ImFontConfig icon_config         = {};
            icon_config.MergeMode            = true;
            icon_config.PixelSnapH           = true;
            icon_config.FontDataOwnedByAtlas = false;
            icon_config.GlyphMinAdvanceX     = 20.0f * scale;

            static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};

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
    CORE_DEBUG("Font atlas built successfully with icon support (scale=%.2f)", scale);

    return true;
}

UI_State *
ui_init(Arena                   *allocator,
        Dynamic_Array<UI_Layer> *layers,
        UI_Theme                 theme,
        PFN_menu_callback        menu_callback,
        String                   app_name,
        Platform_State          *plat_state,
        void                    *global_client_state)
{
    UI_State *state = push_struct(allocator, UI_State);

    state->current_theme       = theme;
    state->menu_callback       = menu_callback;
    state->app_name            = C_STR(app_name);
    state->is_initialized      = true;
    state->platform            = plat_state;
    state->layers              = layers;
    state->global_client_state = global_client_state;

    load_default_fonts(state);

    ImGuiStyle &style = ImGui::GetStyle();

    ui_themes_apply(state->current_theme, style);

    ui_titlebar_setup(state, C_STR(app_name));

    // Register settings handler to persist OS window size in imgui.ini
    ImGuiSettingsHandler wh;
    wh.TypeName  = "AppWindow";
    wh.TypeHash  = ImHashStr("AppWindow");
    wh.UserData  = (void *)plat_state->window;
    wh.ReadOpenFn = [](ImGuiContext *, ImGuiSettingsHandler *, const char *)
        -> void * { return (void *)1; };
    wh.ReadLineFn = [](ImGuiContext *,
                       ImGuiSettingsHandler *handler,
                       void *,
                       const char          *line) {
        SDL_Window *win = (SDL_Window *)handler->UserData;
        int         a, b;
        if (sscanf(line, "Size=%d,%d", &a, &b) == 2)
            SDL_SetWindowSize(win, a, b);
        else if (sscanf(line, "Pos=%d,%d", &a, &b) == 2)
            SDL_SetWindowPosition(win, a, b);
    };
    wh.WriteAllFn = [](ImGuiContext *,
                        ImGuiSettingsHandler *handler,
                        ImGuiTextBuffer      *buf) {
        SDL_Window *win = (SDL_Window *)handler->UserData;
        int         w, h, x, y;
        SDL_GetWindowSize(win, &w, &h);
        SDL_GetWindowPosition(win, &x, &y);
        buf->appendf("[AppWindow][Main]\nSize=%d,%d\nPos=%d,%d\n\n", w, h, x, y);
    };
    ImGui::GetCurrentContext()->SettingsHandlers.push_back(wh);

    for (UI_Layer &layer : *layers)
    {
        if (layer.on_attach)
            layer.on_attach(layer.state);
    }

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
