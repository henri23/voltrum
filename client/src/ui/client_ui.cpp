#include "client_ui.hpp"

// Direct ImGui access (now available as public dependency from core)
#include <core/logger.hpp>
#include <ui/ui.hpp>

#include <imgui.h>

internal_var b8 show_demo_window;

// Component implementations (migrated from core/src/ui/ui_components.cpp)
void client_ui_render_voltrum_window(void* user_data) {

    ImGui::Begin("Voltrum Engine");

    // Engine info section
    ImGui::SeparatorText("Engine Information");
    ImGui::Text("Voltrum Game Engine");
    ImGui::Text("Version: 1.0.0-dev");
    ImGui::Text("Architecture: Vulkan + ImGui + SDL3");

    ImGui::Spacing();

    // UI Controls section
    ImGui::SeparatorText("UI Controls");
    // Note: Performance window is controlled by core, not client
    // Could add a callback to core to toggle it if needed

    ImGui::Spacing();

    local_persist f32 slider_value = 0.0f;
    local_persist ImVec4 clear_color = {};
    local_persist u32 counter = 0;

    // Interactive controls section
    ImGui::SeparatorText("Interactive Controls");
    ImGui::SliderFloat("Test Slider", &slider_value, 0.0f, 1.0f);
    ImGui::ColorEdit3("Clear Color", (float*)&clear_color);

    if (ImGui::Button("Test Button")) {
        counter++;
        CORE_INFO("Button clicked! Count: %d", counter);
    }

    ImGui::SameLine();
    ImGui::Text("Clicks: %d", counter);

    ImGui::Spacing();

    // System info section
    ImGui::SeparatorText("System Information");
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("Frame Time: %.3f ms", 1000.0f / io.Framerate);
    ImGui::Text("FPS: %.1f", io.Framerate);
    ImGui::Text("Vertices: %d", io.MetricsRenderVertices);
    ImGui::Text("Indices: %d", io.MetricsRenderIndices);

    // Show demo window if enabled
    if (show_demo_window) {
        ImGui::ShowDemoWindow();
    }
    ImGui::End();
}

// Menu system implementation (migrated from core/src/ui/ui_components.cpp)
void client_ui_render_menus(void* user_data) {

    // File menu
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New", "Ctrl+N")) {
            CORE_DEBUG("File -> New selected");
        }
        if (ImGui::MenuItem("Open", "Ctrl+O")) {
            CORE_DEBUG("File -> Open selected");
        }
        if (ImGui::MenuItem("Save", "Ctrl+S")) {
            CORE_DEBUG("File -> Save selected");
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Exit", "Alt+F4")) {
            CORE_DEBUG("File -> Exit selected");
            // TODO: Request application exit
        }
        ImGui::EndMenu();
    }

    // View menu
    if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Voltrum Window", nullptr, nullptr);
        ImGui::MenuItem("Demo Window", nullptr, &show_demo_window);

        ImGui::Separator();

        // Theme submenu
        if (ImGui::BeginMenu("Theme")) {
            if (ImGui::MenuItem("Dark")) {
                ui_set_theme(UI_Theme::DARK);
                CORE_DEBUG("Theme changed to Dark");
            }
            if (ImGui::MenuItem("Light")) {
                ui_set_theme(UI_Theme::LIGHT);
                CORE_DEBUG("Theme changed to Light");
            }
            if (ImGui::MenuItem("Catppuccin Mocha")) {
                ui_set_theme(UI_Theme::CATPPUCCIN_MOCHA);
                CORE_DEBUG("Theme changed to Catppuccin Mocha");
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }

    // Help menu
    if (ImGui::BeginMenu("Help")) {
        if (ImGui::MenuItem("About")) {
            CORE_DEBUG("Help -> About selected");
            // TODO: Show about dialog
        }
        ImGui::EndMenu();
    }
}
