#include "app_viewport_layer.hpp"
// #include "ui/ui_viewport.hpp"  // Commented out for UI rewrite
#include "core/logger.hpp"
#include <imgui.h>  // For ImGui calls below

b8 app_viewport_layer_initialize() {
    CORE_DEBUG("Initializing app viewport layer...");

    // Commented out for UI rewrite
    // Initialize the viewport UI component
    // if (!ui_viewport_initialize()) {
    //     CORE_ERROR("Failed to initialize viewport UI");
    //     return false;
    // }

    CORE_INFO("App viewport layer initialized successfully");
    return true;
}

void app_viewport_layer_shutdown() {
    CORE_DEBUG("Shutting down app viewport layer...");
    // ui_viewport_shutdown();  // Commented out for UI rewrite
}

void app_viewport_layer_render(void* component_state) {
    // Commented out for UI rewrite
    // Render the viewport - this will create the 2D workspace window
    // ui_viewport_draw(component_state);

    // Example: Add some additional CAD-specific UI elements
    if (ImGui::Begin("CAD Tools")) {
        ImGui::Text("CAD Workspace Tools");
        ImGui::Separator();

        if (ImGui::Button("Line Tool")) {
            CORE_DEBUG("Line tool selected");
        }

        if (ImGui::Button("Rectangle Tool")) {
            CORE_DEBUG("Rectangle tool selected");
        }

        if (ImGui::Button("Circle Tool")) {
            CORE_DEBUG("Circle tool selected");
        }

        ImGui::Separator();

        if (ImGui::Button("Select Tool")) {
            CORE_DEBUG("Select tool selected");
        }

        if (ImGui::Button("Move Tool")) {
            CORE_DEBUG("Move tool selected");
        }
    }
    ImGui::End();

    // Properties panel
    if (ImGui::Begin("Properties")) {
        ImGui::Text("Object Properties");
        ImGui::Separator();
        ImGui::Text("No object selected");
    }
    ImGui::End();

    // Layers panel
    if (ImGui::Begin("Layers")) {
        ImGui::Text("Drawing Layers");
        ImGui::Separator();

        if (ImGui::Selectable("Layer 1", true)) {
            CORE_DEBUG("Layer 1 selected");
        }

        if (ImGui::Selectable("Layer 2", false)) {
            CORE_DEBUG("Layer 2 selected");
        }
    }
    ImGui::End();
}