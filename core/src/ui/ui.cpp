#include "ui.hpp"
#include "ui_context.hpp"
#include "ui_titlebar.hpp"

#include "core/logger.hpp"
#include "systems/resource_system.hpp"

#include <SDL3/SDL.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

INTERNAL_FUNC void ui_dockspace_render(UI_Context* context) {
    UI_Dockspace_State* dockspace = &context->dockspace;

    dockspace->window_began = false;

    if (dockspace->dockspace_id == 0) {
        dockspace->dockspace_id = ImGui::GetID("MainDockspace");
        CORE_DEBUG("Generated dockspace ID: %u", dockspace->dockspace_id);
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos;
    ImVec2 work_size = viewport->WorkSize;

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

    const char* window_name = "DockSpace";
    ImGui::Begin(window_name, &dockspace->dockspace_open, window_flags);
    dockspace->window_began = true;

    ImGui::PopStyleVar(3);

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiStyle& style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 300.0f;

        ImGui::DockSpace(dockspace->dockspace_id);

        style.WindowMinSize.x = minWinSizeX;
    } else {
        CORE_ERROR("ImGui docking is not enabled!");
    }

    if (dockspace->window_began) {
        ImGui::End();
        dockspace->window_began = false;
    }
}

INTERNAL_FUNC b8 load_default_fonts(UI_Context* context) {
    ImGuiIO& io = ImGui::GetIO();

    constexpr const char* style_names[] = {"normal",
        "italic",
        "bold_normal",
        "bold_italic"};

    for (u8 s = 0; s < (u8)Font_Style::MAX_COUNT; ++s) {
        char path[128];
        u32 i = 0;

        const char* prefix = "jetbrains/jetbrains_";
        for (u32 j = 0; prefix[j] != '\0'; ++j)
            path[i++] = prefix[j];

        for (u32 j = 0; style_names[s][j] != '\0'; ++j)
            path[i++] = style_names[s][j];

        path[i] = '\0';

        Resource resource = {};
        if (!resource_system_load(path, Resource_Type::FONT, &resource)) {
            CORE_ERROR("Failed to load font: %s", path);
            context->fonts[s] = nullptr;
            continue;
        }

        ImFontConfig config = {};
        config.FontDataOwnedByAtlas = false;

        context->fonts[s] = io.Fonts->AddFontFromMemoryTTF(resource.data,
            resource.data_size,
            20.0f,
            &config);

        resource_system_unload(&resource);

        if (context->fonts[s]) {
            CORE_DEBUG("Loaded font: %s at %.0fpt", path, 20.0f);
        }
    }

    if (!io.Fonts->Build()) {
        CORE_ERROR("Failed to build font atlas");
        return false;
    }

    io.FontDefault = context->fonts[(u8)Font_Style::NORMAL];

    return true;
}

b8 ui_initialize(UI_Context* context,
    UI_Layer* layers,
    u32 layer_count,
    UI_Theme theme,
    PFN_menu_callback menu_callback,
    const char* app_name,
    SDL_Window* window) {

    context->current_theme = theme;
    context->menu_callback = menu_callback;
    context->app_name = app_name;
    context->is_initialized = true;

    load_default_fonts(context);

    ImGuiStyle& style = ImGui::GetStyle();
    ui_themes_apply(context->current_theme, style);

    ui_titlebar_setup(context, app_name);

    for (u32 i = 0; i < layer_count; ++i) {
        UI_Layer* layer = &layers[i];
        if (layer->on_attach)
            layer->on_attach(layer);
    }

    return true;
}

void ui_shutdown(UI_Context* context, UI_Layer* layers, u32 layer_count) {
    for (u32 i = 0; i < layer_count; ++i) {
        UI_Layer* layer = &layers[i];
        if (layer->on_detach)
            layer->on_detach(layer);
    }
}

void ui_update_layers(UI_Context* context,
    UI_Layer* layers,
    u32 layer_count,
    f32 delta_t) {
    for (u32 i = 0; i < layer_count; ++i) {
        UI_Layer* layer = &layers[i];
        if (layer->on_update)
            layer->on_update(layer, delta_t);
    }
}

ImDrawData* ui_draw_layers(UI_Context* context,
    UI_Layer* layers,
    u32 layer_count,
    f32 delta_t) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ui_titlebar_draw(context);
    ui_dockspace_render(context);

    ImGui::ShowDemoWindow();

    for (u32 i = 0; i < layer_count; ++i) {
        UI_Layer* layer = &layers[i];
        if (layer->on_render)
            layer->on_render(layer, delta_t);
    }

    ImGui::Render();

    return ImGui::GetDrawData();
}
