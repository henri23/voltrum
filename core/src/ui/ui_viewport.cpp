#include "ui_viewport.hpp"
#include "core/logger.hpp"
#include "defines.hpp"
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "renderer/vulkan/vulkan_backend.hpp"
#include "renderer/vulkan/vulkan_types.hpp"
#include <cmath>

// Global viewport state
internal_var Viewport_State viewport_state;

b8 ui_viewport_initialize() {
    CORE_DEBUG("Initializing viewport layer...");

    // Initialize viewport state with default values
    viewport_state.pan_offset = ImVec2(0.0f, 0.0f);
    viewport_state.zoom_level = 1.0f;

    viewport_state.show_grid = true;
    viewport_state.grid_size = 50.0f;
    viewport_state.grid_subdivisions = 5.0f;

    viewport_state.viewport_pos = ImVec2(0.0f, 0.0f);
    viewport_state.viewport_size = ImVec2(800.0f, 600.0f);

    viewport_state.is_panning = false;
    viewport_state.is_zooming = false;
    viewport_state.last_mouse_pos = ImVec2(0.0f, 0.0f);

    // Set default colors (dark theme)
    viewport_state.grid_color = IM_COL32(80, 80, 80, 255);
    viewport_state.grid_major_color = IM_COL32(120, 120, 120, 255);
    viewport_state.background_color = IM_COL32(45, 45, 45, 255);

    CORE_INFO("Viewport layer initialized successfully");
    return true;
}

void ui_viewport_shutdown() {
    CORE_DEBUG("Shutting down viewport layer...");
    // Nothing to clean up for now
}

void ui_viewport_cleanup_vulkan_resources(Vulkan_Context* context) {
    CORE_DEBUG("Cleaning up viewport Vulkan resources...");

    // Clean up viewport descriptor sets (all framebuffers)
    if (context) {
        for (u32 i = 0; i < context->main_target.framebuffer_count; ++i) {
            if (context->main_target.descriptor_sets[i] != VK_NULL_HANDLE) {
                ImGui_ImplVulkan_RemoveTexture(context->main_target.descriptor_sets[i]);
                context->main_target.descriptor_sets[i] = VK_NULL_HANDLE;
            }
        }
        CORE_DEBUG("Viewport descriptor sets cleaned up");
    }
}

void ui_viewport_draw(void* component_state) {
    // Create the main viewport window
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    if (ImGui::Begin("2D Workspace", nullptr, window_flags)) {
        // Get the current window position and size
        viewport_state.viewport_pos = ImGui::GetWindowPos();
        viewport_state.viewport_size = ImGui::GetWindowSize();

        // Calculate the drawing area (excluding window decorations)
        ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
        ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
        ImVec2 canvas_p1 =
            ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

        // Update viewport drawing area
        viewport_state.viewport_pos = canvas_p0;
        viewport_state.viewport_size = canvas_sz;

        // Resize main renderer target to match viewport size
        u32 target_width = (u32)fmaxf(canvas_sz.x, 1.0f);
        u32 target_height = (u32)fmaxf(canvas_sz.y, 1.0f);

        vulkan_resize_main_target(target_width, target_height);

        // Get the main renderer texture
        VkDescriptorSet main_texture = vulkan_get_main_texture();

        // Display the off-screen rendered content as a texture
        if (main_texture != VK_NULL_HANDLE) {
            // CORE_DEBUG("Displaying viewport texture with descriptor set: %p", (void*)main_texture);
            ImGui::Image((ImTextureID)main_texture, canvas_sz);
        } else {
            CORE_ERROR("No viewport texture available - descriptor set is NULL");
            // Fallback: display a placeholder
            ImGui::Text("Viewport Loading...");
        }

        // Handle input for the image
        ImGuiIO& io = ImGui::GetIO();
        const bool is_hovered = ImGui::IsItemHovered();
        const bool is_active = ImGui::IsItemActive();

        ImVec2 mouse_pos_in_canvas =
            ImVec2(io.MousePos.x - canvas_p0.x, io.MousePos.y - canvas_p0.y);

        // Pan handling
        if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
            viewport_state.is_panning = true;
            viewport_state.last_mouse_pos = mouse_pos_in_canvas;
        }

        if (viewport_state.is_panning) {
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
                ImVec2 delta = ImVec2(mouse_pos_in_canvas.x -
                                          viewport_state.last_mouse_pos.x,
                    mouse_pos_in_canvas.y - viewport_state.last_mouse_pos.y);
                ui_viewport_pan(&viewport_state, delta);
                viewport_state.last_mouse_pos = mouse_pos_in_canvas;
            } else {
                viewport_state.is_panning = false;
            }
        }

        // Zoom handling
        if (is_hovered && io.MouseWheel != 0.0f) {
            ImVec2 zoom_center = mouse_pos_in_canvas;
            f32 zoom_delta = io.MouseWheel * 0.1f;
            ui_viewport_zoom(&viewport_state, zoom_delta, zoom_center);
        }

        // Display viewport info
        ImGui::Text("Zoom: %.1f%% | Pan: (%.1f, %.1f)",
            viewport_state.zoom_level * 100.0f,
            viewport_state.pan_offset.x,
            viewport_state.pan_offset.y);
    }
    ImGui::End();

    // Viewport controls window
    if (ImGui::Begin("Viewport Controls")) {
        ImGui::Text("Viewport Settings");
        ImGui::Separator();

        if (ImGui::Button("Reset View")) {
            ui_viewport_reset_view(&viewport_state);
        }

        ImGui::SameLine();
        if (ImGui::Button("Fit to Grid")) {
            // Center view on origin
            viewport_state.pan_offset =
                ImVec2(viewport_state.viewport_size.x * 0.5f,
                    viewport_state.viewport_size.y * 0.5f);
        }

        ImGui::Checkbox("Show Grid", &viewport_state.show_grid);

        if (viewport_state.show_grid) {
            ImGui::SliderFloat("Grid Size",
                &viewport_state.grid_size,
                10.0f,
                200.0f,
                "%.1f");
            ImGui::SliderFloat("Subdivisions",
                &viewport_state.grid_subdivisions,
                2.0f,
                10.0f,
                "%.0f");
        }

        ImGui::Text("Controls:");
        ImGui::BulletText("Middle mouse: Pan");
        ImGui::BulletText("Mouse wheel: Zoom");
    }
    ImGui::End();
}

ImVec2 ui_viewport_world_to_screen(const Viewport_State* viewport,
    ImVec2 world_pos) {
    return ImVec2(viewport->viewport_pos.x +
                      (world_pos.x + viewport->pan_offset.x) *
                          viewport->zoom_level,
        viewport->viewport_pos.y +
            (world_pos.y + viewport->pan_offset.y) * viewport->zoom_level);
}

ImVec2 ui_viewport_screen_to_world(const Viewport_State* viewport,
    ImVec2 screen_pos) {
    return ImVec2((screen_pos.x - viewport->viewport_pos.x) /
                          viewport->zoom_level -
                      viewport->pan_offset.x,
        (screen_pos.y - viewport->viewport_pos.y) / viewport->zoom_level -
            viewport->pan_offset.y);
}

void ui_viewport_pan(Viewport_State* viewport, ImVec2 delta) {
    viewport->pan_offset.x += delta.x / viewport->zoom_level;
    viewport->pan_offset.y += delta.y / viewport->zoom_level;
}

void ui_viewport_zoom(Viewport_State* viewport,
    f32 zoom_delta,
    ImVec2 zoom_center) {
    f32 old_zoom = viewport->zoom_level;
    viewport->zoom_level =
        fmaxf(0.1f, fminf(10.0f, viewport->zoom_level + zoom_delta));

    // Adjust pan to zoom around the zoom center
    f32 zoom_ratio = viewport->zoom_level / old_zoom;
    ImVec2 center_world = ui_viewport_screen_to_world(viewport, zoom_center);

    // Calculate new pan offset to keep the zoom center in the same screen
    // position
    viewport->pan_offset.x =
        zoom_center.x / viewport->zoom_level - center_world.x;
    viewport->pan_offset.y =
        zoom_center.y / viewport->zoom_level - center_world.y;
}

void ui_viewport_reset_view(Viewport_State* viewport) {
    viewport->pan_offset = ImVec2(viewport->viewport_size.x * 0.5f,
        viewport->viewport_size.y * 0.5f);
    viewport->zoom_level = 1.0f;
}

void ui_viewport_draw_grid(const Viewport_State* viewport,
    ImDrawList* draw_list) {
    if (!viewport->show_grid)
        return;

    f32 grid_step = viewport->grid_size * viewport->zoom_level;

    // Don't draw grid if it's too small or too large
    if (grid_step < 5.0f || grid_step > 500.0f)
        return;

    ImVec2 canvas_p0 = viewport->viewport_pos;
    ImVec2 canvas_p1 = ImVec2(canvas_p0.x + viewport->viewport_size.x,
        canvas_p0.y + viewport->viewport_size.y);

    // Calculate grid offset
    f32 grid_offset_x =
        fmodf(viewport->pan_offset.x * viewport->zoom_level, grid_step);
    f32 grid_offset_y =
        fmodf(viewport->pan_offset.y * viewport->zoom_level, grid_step);

    // Draw vertical lines
    for (f32 x = canvas_p0.x + grid_offset_x; x < canvas_p1.x; x += grid_step) {
        // Determine if this is a major grid line
        s32 line_index = (s32)((x - canvas_p0.x - grid_offset_x) / grid_step);
        b8 is_major = (line_index % (s32)viewport->grid_subdivisions) == 0;

        ImU32 color =
            is_major ? viewport->grid_major_color : viewport->grid_color;
        draw_list->AddLine(ImVec2(x, canvas_p0.y),
            ImVec2(x, canvas_p1.y),
            color,
            is_major ? 1.5f : 1.0f);
    }

    // Draw horizontal lines
    for (f32 y = canvas_p0.y + grid_offset_y; y < canvas_p1.y; y += grid_step) {
        // Determine if this is a major grid line
        s32 line_index = (s32)((y - canvas_p0.y - grid_offset_y) / grid_step);
        b8 is_major = (line_index % (s32)viewport->grid_subdivisions) == 0;

        ImU32 color =
            is_major ? viewport->grid_major_color : viewport->grid_color;
        draw_list->AddLine(ImVec2(canvas_p0.x, y),
            ImVec2(canvas_p1.x, y),
            color,
            is_major ? 1.5f : 1.0f);
    }

    // Draw origin axes if visible
    ImVec2 origin_screen =
        ui_viewport_world_to_screen(viewport, ImVec2(0.0f, 0.0f));

    // X-axis (red)
    if (origin_screen.y >= canvas_p0.y && origin_screen.y <= canvas_p1.y) {
        draw_list->AddLine(ImVec2(canvas_p0.x, origin_screen.y),
            ImVec2(canvas_p1.x, origin_screen.y),
            IM_COL32(255, 100, 100, 255),
            2.0f);
    }

    // Y-axis (green)
    if (origin_screen.x >= canvas_p0.x && origin_screen.x <= canvas_p1.x) {
        draw_list->AddLine(ImVec2(origin_screen.x, canvas_p0.y),
            ImVec2(origin_screen.x, canvas_p1.y),
            IM_COL32(100, 255, 100, 255),
            2.0f);
    }
}
