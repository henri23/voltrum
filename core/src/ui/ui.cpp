#include "ui.hpp"

#include "core/logger.hpp"
#include "systems/resource_system.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

struct UI_State {
    UI_Theme current_theme;
    PFN_menu_callback menu_callback;
    const char* app_name;
    b8 is_initialized;

    unsigned int dockspace_id;
    b8 dockspace_open;
    b8 window_began; // Track if ImGui::Begin() was called this frame

    ImFont* fonts[(u8)Font_Style::MAX_COUNT];
};

internal_var UI_State state;

INTERNAL_FUNC void ui_dockspace_render() {
    state.window_began = false;

    if (state.dockspace_id == 0) {
        state.dockspace_id = ImGui::GetID("MainDockspace");
        CORE_DEBUG("Generated dockspace ID: %u", state.dockspace_id);
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos;
    ImVec2 work_size = viewport->WorkSize;

    f32 titlebar_height = 50.0f;
    work_pos.y += titlebar_height;
    work_size.y -= titlebar_height;

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
    ImGui::Begin(window_name, &state.dockspace_open, window_flags);
    state.window_began = true;

    ImGui::PopStyleVar(3);

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiStyle& style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 300.0f;

        ImGui::DockSpace(state.dockspace_id);

        style.WindowMinSize.x = minWinSizeX;
    } else {
        CORE_ERROR("ImGui docking is not enabled!");
    }

    if (state.window_began) {
        ImGui::End();
        state.window_began = false;
    }
}

INTERNAL_FUNC b8 load_default_fonts() {
    ImGuiIO& io = ImGui::GetIO();

    constexpr const char* style_names[] = {"normal",
        "italic",
        "bold_normal",
        "bold_italic"};

    // Loop through all combinations and load fonts
    for (u8 s = 0; s < (u8)Font_Style::MAX_COUNT; ++s) {
        // Build resource path: "jetbrains/jetbrains_{style}"
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
            state.fonts[s] = nullptr;
            continue;
        }

        ImFontConfig config = {};
        config.FontDataOwnedByAtlas = false;

        state.fonts[s] = io.Fonts->AddFontFromMemoryTTF(resource.data,
            resource.data_size,
            20.0f,
            &config);

        resource_system_unload(&resource);

        if (state.fonts[s]) {
            CORE_DEBUG("Loaded font: %s at %.0fpt", path, 20.0f);
        }
    }

    if (!io.Fonts->Build()) {
        CORE_ERROR("Failed to build font atlas");
        return false;
    }

    io.FontDefault = state.fonts[(u8)Font_Style::NORMAL];

    return true;
}

b8 ui_initialize(UI_Layer* layers,
    u32 layer_count,
    UI_Theme theme,
    PFN_menu_callback menu_callback,
    const char* app_name,
    SDL_Window* window) {

    load_default_fonts();

    for (u32 i = 0; i < layer_count; ++i) {
        UI_Layer* layer = &layers[i];
        if (layer->on_attach)
            layer->on_attach(layer);
    }

    return true;
}

void ui_shutdown(UI_Layer* layers, u32 layer_count) {
    for (u32 i = 0; i < layer_count; ++i) {
        UI_Layer* layer = &layers[i];
        if (layer->on_detach)
            layer->on_detach(layer);
    }
}

ImDrawData* ui_draw_layers(UI_Layer* layers, u32 layer_count, f32 delta_t) {

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // Render dockspace
    ui_dockspace_render();

    // Render custom titlebar
    // ui_titlebar_draw();

    ImGui::ShowDemoWindow();

    for (u32 i = 0; i < layer_count; ++i) {
        UI_Layer* layer = &layers[i];
        if (layer->on_render)
            layer->on_render(layer, delta_t);
    }

    ImGui::Render();

    return ImGui::GetDrawData();
}
