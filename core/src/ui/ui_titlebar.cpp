#include "ui_titlebar.hpp"
#include "resources/loaders/image_loader.hpp"
#include "renderer/renderer_frontend.hpp"
#include "ui.hpp"
#include "ui_themes.hpp"

#include "core/asserts.hpp"
#include "core/logger.hpp"
#include "stb_image.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_internal.h"
#include "platform/platform.hpp"

// Internal titlebar state
struct Titlebar_State {
    const char* title_text;

    // UI Image resources (stack-allocated)
    UI_Image_Resource app_icon;
    UI_Image_Resource minimize_icon;
    UI_Image_Resource maximize_icon;
    UI_Image_Resource restore_icon;
    UI_Image_Resource close_icon;

    ImVec2 titlebar_min;
    ImVec2 titlebar_max;

    PFN_menu_callback callback;

    // Titlebar hover state for native dragging
    b8 is_titlebar_hovered;
    b8 is_menu_hovered;
};

internal_var Titlebar_State state = {};

// Titlebar constants

// Walnut-style menu helpers
INTERNAL_FUNC ImRect rect_offset(const ImRect& rect, float x, float y);
INTERNAL_FUNC bool begin_menubar(const ImRect& barRectangle);
INTERNAL_FUNC void end_menubar();

// Internal functions
INTERNAL_FUNC b8 load_ui_image(const char* image_name, UI_Image_Resource* image_resource);
INTERNAL_FUNC void draw_titlebar_background();
INTERNAL_FUNC void draw_titlebar_gradient();
INTERNAL_FUNC void draw_titlebar_logo();
INTERNAL_FUNC void draw_titlebar_menus(PFN_menu_callback callback);
INTERNAL_FUNC void draw_titlebar_text();
INTERNAL_FUNC void draw_titlebar_buttons();
INTERNAL_FUNC void handle_titlebar_hover();

INTERNAL_FUNC b8 draw_titlebar_button(const char* icon,
    ImVec2 pos,
    ImVec2 size);

INTERNAL_FUNC b8 draw_titlebar_image_button(UI_Image_Resource* image_resource,
    const char* fallback_text,
    ImVec2 pos,
    ImVec2 size);

// Setup titlebar with configuration
void ui_titlebar_setup(PFN_menu_callback callback, const char* app_name) {
    // Setup configuration
    state.title_text = app_name;
    state.callback = callback;

    // Load icons using image loader and renderer directly
    b8 icons_success = true;
    icons_success &= load_ui_image("voltrum_icon", &state.app_icon);
    icons_success &= load_ui_image("window_minimize", &state.minimize_icon);
    icons_success &= load_ui_image("window_maximize", &state.maximize_icon);
    icons_success &= load_ui_image("window_restore", &state.restore_icon);
    icons_success &= load_ui_image("window_close", &state.close_icon);

    if (icons_success) {
        CORE_INFO("All titlebar icons loaded successfully");
    } else {
        CORE_WARN("Some titlebar icons failed to load, using fallback text");
    }
}

void ui_titlebar_cleanup_renderer_resources() {
    CORE_DEBUG("Cleaning up titlebar renderer resources...");

    // Clean up UI image resources directly through renderer
    // Check if resources are valid before destroying
    if (state.app_icon.is_valid) {
        renderer_destroy_ui_image(&state.app_icon);
    }

    if (state.minimize_icon.is_valid) {
        renderer_destroy_ui_image(&state.minimize_icon);
    }

    if (state.maximize_icon.is_valid) {
        renderer_destroy_ui_image(&state.maximize_icon);
    }

    if (state.restore_icon.is_valid) {
        renderer_destroy_ui_image(&state.restore_icon);
    }

    if (state.close_icon.is_valid) {
        renderer_destroy_ui_image(&state.close_icon);
    }

    CORE_DEBUG("Titlebar renderer resources cleanup completed");
}


void ui_titlebar_draw() {

    // Colors are now fetched directly from theme in render functions

    static b8 render_debug_logged = false;
    if (!render_debug_logged) {
        CORE_DEBUG("Custom titlebar rendering started");
        render_debug_logged = true;
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 window_pos = viewport->Pos;
    ImVec2 window_size = viewport->Size;

    state.titlebar_min = window_pos;
    state.titlebar_max =
        ImVec2(window_pos.x + window_size.x, window_pos.y + TITLEBAR_HEIGHT);

    // Create invisible window for titlebar
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(window_size.x, TITLEBAR_HEIGHT));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDocking;

    if (ImGui::Begin("##CustomTitlebar", nullptr, flags)) {
        // Disable docking for the entire titlebar area
        ImGui::GetCurrentWindow()->DockNode = nullptr; // Ensure no dock node

        draw_titlebar_background();
        draw_titlebar_gradient();
        draw_titlebar_logo();
        draw_titlebar_menus(state.callback);
        draw_titlebar_text();
        draw_titlebar_buttons();
        handle_titlebar_hover();
    }
    ImGui::End();
}

// Internal function implementations
INTERNAL_FUNC const UI_Theme_Palette& get_current_palette() {
    extern UI_Theme ui_get_current_theme(); // Internal function from ui.cpp
    UI_Theme current_theme = ui_get_current_theme();
    return ui_themes_get_palette(current_theme);
}

INTERNAL_FUNC void draw_titlebar_background() {
    const UI_Theme_Palette& palette = get_current_palette();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(state.titlebar_min,
        state.titlebar_max,
        palette.titlebar);
}

INTERNAL_FUNC void draw_titlebar_logo() {
    UI_Image_Resource* app_icon = state.app_icon.is_valid ? &state.app_icon : nullptr;

    // Fixed logo size (decoupled from titlebar height)
    f32 logo_size = 50.0f; // Fixed size for larger Voltrum icon
    f32 logo_margin = 4.0f;
    f32 logo_top_padding =
        2.0f; // Same top padding as menu entries and window buttons

    // Position: fixed distance from top edge (not centered)
    ImVec2 logo_pos = ImVec2(state.titlebar_min.x + logo_margin,
        state.titlebar_min.y + logo_top_padding);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    if (app_icon && app_icon->is_valid) {
        // Get descriptor set directly from resource
        VkDescriptorSet descriptor_set = (VkDescriptorSet)app_icon->descriptor_set;
        if (descriptor_set != VK_NULL_HANDLE) {
            // Render the scalable logo image
            draw_list->AddImage((ImTextureID)(intptr_t)descriptor_set,
                logo_pos,
                ImVec2(logo_pos.x + logo_size, logo_pos.y + logo_size),
                ImVec2(0, 0),
                ImVec2(1, 1),
                IM_COL32_WHITE);
        } else {
            // GPU resource not ready, use fallback
            goto logo_fallback;
        }
    } else {
logo_fallback:
        // Fallback: draw a scalable placeholder
        const UI_Theme_Palette& palette = get_current_palette();
        draw_list->AddRect(logo_pos,
            ImVec2(logo_pos.x + logo_size, logo_pos.y + logo_size),
            palette.text,
            2.0f);

        // Add "P" text as placeholder
        ImVec2 text_size = ImGui::CalcTextSize("P");
        ImVec2 text_pos = ImVec2(logo_pos.x + (logo_size - text_size.x) * 0.5f,
            logo_pos.y + (logo_size - text_size.y) * 0.5f);
        draw_list->AddText(text_pos, palette.text, "P");
    }
}

INTERNAL_FUNC void draw_titlebar_menus(PFN_menu_callback callback) {
    if (!callback) {
        state.is_menu_hovered = false;
        return;
    }

    // Safety check: ensure ImGui context is ready
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    if (!ctx) {
        state.is_menu_hovered = false;
        return;
    }

    // Use Walnut's exact approach
    ImGui::SetItemAllowOverlap();
    const float logoHorizontalOffset =
        4.0f + 50.0f + 4.0f;           // logo_margin + logo_size + spacing
    const float menuTopPadding = 2.0f; // Same as logo and buttons
    ImGui::SetCursorPos(ImVec2(logoHorizontalOffset, menuTopPadding));

    // Create menu rectangle like Walnut
    const ImRect menuBarRect = {ImGui::GetCursorPos(),
        {ImGui::GetContentRegionAvail().x + ImGui::GetCursorScreenPos().x,
            ImGui::GetFrameHeightWithSpacing()}};

    ImGui::BeginGroup();

    // Store cursor position before rendering menus
    ImVec2 menu_start_pos = ImGui::GetCursorScreenPos();

    if (begin_menubar(menuBarRect)) {
        callback(nullptr);
    }

    // Store cursor position after rendering menus to get actual width
    ImVec2 menu_end_pos = ImGui::GetCursorScreenPos();

    end_menubar();
    ImGui::EndGroup();

    // Simple approach like Walnut: check if menu group is hovered or any popup
    // is open
    state.is_menu_hovered =
        ImGui::IsItemHovered() ||
        ImGui::IsPopupOpen("",
            ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
}

INTERNAL_FUNC void draw_titlebar_text() {
    if (!state.title_text) {
        return;
    }

    const UI_Theme_Palette& palette = get_current_palette();

    // Calculate text size for centering
    ImVec2 text_size = ImGui::CalcTextSize(state.title_text);
    f32 titlebar_width = state.titlebar_max.x - state.titlebar_min.x;

    // Absolute center positioning (ignoring other elements)
    f32 text_x = state.titlebar_min.x + (titlebar_width - text_size.x) * 0.5f;

    // Check for overlap with menu entries (if shown)
    // Calculate menu area bounds
    f32 logo_size = 50.0f;
    f32 logo_margin = 4.0f;
    f32 menu_start_x = state.titlebar_min.x + logo_margin + logo_size + 4.0f;

    // Estimate total menu width (File, View, Help with padding)
    f32 file_width = ImGui::CalcTextSize("File").x + 16; // text + padding
    f32 view_width = ImGui::CalcTextSize("View").x + 16; // text + padding
    f32 help_width = ImGui::CalcTextSize("Help").x + 16; // text + padding
    f32 menu_end_x = menu_start_x + file_width + view_width + help_width;

    // Calculate title bounds
    f32 title_start_x = text_x;
    f32 title_end_x = text_x + text_size.x;

    // Check if title overlaps with menu area
    b8 overlaps = (title_start_x < menu_end_x) && (title_end_x > menu_start_x);

    if (overlaps) {
        // Hide title when it would overlap with menus
        return;
    }

    // Fixed distance from top edge
    f32 text_top_padding = 6.0f;
    f32 text_y = state.titlebar_min.y + text_top_padding;

    ImVec2 text_pos = ImVec2(text_x, text_y);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddText(text_pos, palette.text, state.title_text);
}

INTERNAL_FUNC void draw_titlebar_buttons() {
    // Fixed button size (not scaling with titlebar height)
    f32 button_size = 26.0f; // Fixed size for consistent window controls
    f32 button_spacing = 2.0f;
    f32 right_margin = 4.0f;
    f32 top_padding = 2.0f; // Fixed distance from top edge

    f32 current_x = state.titlebar_max.x - right_margin;

    // Close button (rightmost) - always show
    {
        current_x -= button_size;
        ImVec2 close_pos = ImVec2(current_x,
            state.titlebar_min.y + top_padding); // Top-aligned

        ImVec2 close_size = ImVec2(button_size, button_size);

        UI_Image_Resource* close_icon =
            state.close_icon.is_valid ? &state.close_icon : nullptr;
        if (draw_titlebar_image_button(close_icon,
                "×",
                close_pos,
                close_size)) {
            CORE_DEBUG("Close button clicked - requesting application exit");
            platform_close_window();
        }
        current_x -= button_spacing;
    }

    // Maximize button - always show
    {
        current_x -= button_size;
        ImVec2 max_pos = ImVec2(current_x,
            state.titlebar_min.y + top_padding); // Top-aligned
        ImVec2 max_size = ImVec2(button_size, button_size);

        // Check if window is maximized and use appropriate icon
        b8 is_maximized = platform_is_window_maximized();
        UI_Image_Resource* max_icon =
            is_maximized
                ? (state.restore_icon.is_valid ? &state.restore_icon : nullptr)
                : (state.maximize_icon.is_valid ? &state.maximize_icon : nullptr);
        const char* fallback_text = is_maximized ? "🗗" : "□";

        if (draw_titlebar_image_button(max_icon,
                fallback_text,
                max_pos,
                max_size)) {
            if (is_maximized) {
                CORE_DEBUG("Restore button clicked - restoring window");
                platform_restore_window();
            } else {
                CORE_DEBUG("Maximize button clicked - maximizing window");
                platform_maximize_window();
            }
        }
        current_x -= button_spacing;
    }

    // Minimize button - always show
    {
        current_x -= button_size;
        ImVec2 min_pos = ImVec2(current_x,
            state.titlebar_min.y + top_padding); // Top-aligned

        ImVec2 min_size = ImVec2(button_size, button_size);

        UI_Image_Resource* min_icon =
            state.minimize_icon.is_valid ? &state.minimize_icon : nullptr;

        if (draw_titlebar_image_button(min_icon, "−", min_pos, min_size)) {

            CORE_DEBUG("Minimize button clicked - minimizing window");
            platform_minimize_window();
        }
    }
}

INTERNAL_FUNC b8 draw_titlebar_button(const char* icon,
    ImVec2 pos,
    ImVec2 size) {

    const UI_Theme_Palette& palette = get_current_palette();
    ImGui::SetCursorScreenPos(pos);

    // Create invisible button for interaction
    ImGui::PushID(icon);
    b8 clicked = ImGui::InvisibleButton("##titlebar_btn", size);

    // Get button state for coloring
    u32 button_color = palette.titlebar;

    if (ImGui::IsItemActive()) {
        button_color = palette.highlight;
    } else if (ImGui::IsItemHovered()) {
        button_color = palette.button_hovered;
    }

    // Draw button background
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(pos,
        ImVec2(pos.x + size.x, pos.y + size.y),
        button_color);

    // Draw icon text centered
    ImVec2 text_size = ImGui::CalcTextSize(icon);
    ImVec2 text_pos = ImVec2(pos.x + (size.x - text_size.x) * 0.5f,
        pos.y + (size.y - text_size.y) * 0.5f);
    draw_list->AddText(text_pos, palette.text, icon);

    ImGui::PopID();
    return clicked;
}

INTERNAL_FUNC b8 draw_titlebar_image_button(UI_Image_Resource* image_resource,
    const char* fallback_text,
    ImVec2 pos,
    ImVec2 size) {
    const UI_Theme_Palette& palette = get_current_palette();
    ImGui::SetCursorScreenPos(pos);

    // Create invisible button for interaction
    ImGui::PushID(fallback_text);
    b8 clicked = ImGui::InvisibleButton("##titlebar_img_btn", size);

    // Get button state for coloring
    u32 button_color = palette.titlebar;
    if (ImGui::IsItemActive()) {
        button_color = palette.highlight;
    } else if (ImGui::IsItemHovered()) {
        button_color = palette.button_hovered;
    }

    // Draw button background
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(pos,
        ImVec2(pos.x + size.x, pos.y + size.y),
        button_color);

    if (image_resource && image_resource->is_valid) {
        // Get descriptor set directly from resource
        VkDescriptorSet descriptor_set = (VkDescriptorSet)image_resource->descriptor_set;
        if (descriptor_set != VK_NULL_HANDLE) {
            // Get actual image dimensions to preserve aspect ratio
            u32 original_width = image_resource->width;
            u32 original_height = image_resource->height;

            f32 original_aspect = (f32)original_width / (f32)original_height;

            // Calculate available space for icon (with padding)
            f32 available_width = size.x * 0.7f;  // 70% of button width for icon
            f32 available_height = size.y * 0.7f; // 70% of button height for icon

            // Use icons at their natural size (no scaling unless they're too large)
            f32 scale = 1.0f; // Start with natural size

            // Only scale down if the icon is larger than available space
            if ((f32)original_width > available_width) {
                scale = available_width / (f32)original_width;
            }
            if ((f32)original_height > available_height) {
                f32 height_scale = available_height / (f32)original_height;
                if (height_scale < scale) {
                    scale = height_scale;
                }
            }

            // Calculate final dimensions (natural size or scaled down if needed)
            f32 scaled_width = (f32)original_width * scale;
            f32 scaled_height = (f32)original_height * scale;

            ImVec2 image_size = ImVec2(scaled_width, scaled_height);

            // Special positioning for minimize icon (thin horizontal line)
            f32 vertical_pos;
            if (original_height <= 3 && original_width > original_height * 3) {
                // This is likely a minimize icon (very thin and wide) - align to bottom
                vertical_pos = pos.y + size.y - image_size.y - (size.y * 0.15f); // 15% padding from bottom
            } else {
                // Normal icons - center vertically
                vertical_pos = pos.y + (size.y - image_size.y) * 0.5f;
            }

            ImVec2 image_pos = ImVec2(pos.x + (size.x - image_size.x) * 0.5f, // Center horizontally
                vertical_pos);

            // Render the icon with proper color tinting based on button state
            u32 icon_color = palette.text;
            if (ImGui::IsItemActive()) {
                icon_color = palette.text; // Keep same color when active
            } else if (ImGui::IsItemHovered()) {
                icon_color = IM_COL32_WHITE; // Brighter when hovered
            }

            draw_list->AddImage((ImTextureID)(intptr_t)descriptor_set,
                image_pos,
                ImVec2(image_pos.x + image_size.x, image_pos.y + image_size.y),
                ImVec2(0, 0),
                ImVec2(1, 1),
                icon_color);
        } else {
            // GPU resource not ready, use fallback
            goto button_fallback;
        }
    } else {
button_fallback:
        // Fall back to text if no image resource or GPU resource not ready
        // Debug: Log why we're falling back
        static b8 button_debug_logged = false;
        if (!button_debug_logged) {
            CORE_DEBUG(
                "Button fallback: image_resource=%p, valid=%s",
                (void*)image_resource,
                (image_resource && image_resource->is_valid) ? "true" : "false");
            button_debug_logged = true;
        }

        ImVec2 text_size = ImGui::CalcTextSize(fallback_text);
        ImVec2 text_pos = ImVec2(pos.x + (size.x - text_size.x) * 0.5f,
            pos.y + (size.y - text_size.y) * 0.5f);
        draw_list->AddText(text_pos, palette.text, fallback_text);
    }

    ImGui::PopID();
    return clicked;
}

INTERNAL_FUNC void draw_titlebar_gradient() {
    // Get current theme palette for gradient colors
    const UI_Theme_Palette& palette = get_current_palette();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Calculate gradient area (left 1/4 of titlebar)
    f32 titlebar_width = state.titlebar_max.x - state.titlebar_min.x;
    f32 gradient_width = titlebar_width * 0.25f; // 1/4 of width

    ImVec2 gradient_min = state.titlebar_min;
    ImVec2 gradient_max =
        ImVec2(state.titlebar_min.x + gradient_width, state.titlebar_max.y);

    // Use gradient colors from theme palette
    u32 gradient_start_color = palette.titlebar_gradient_start;
    u32 gradient_end_color = palette.titlebar_gradient_end;

    // Render horizontal gradient
    draw_list->AddRectFilledMultiColor(gradient_min,
        gradient_max,
        gradient_start_color, // Top-left
        gradient_end_color,   // Top-right
        gradient_end_color,   // Bottom-right
        gradient_start_color  // Bottom-left
    );
}

// Walnut-style menu helper implementations
INTERNAL_FUNC ImRect rect_offset(const ImRect& rect, float x, float y) {
    ImRect result = rect;
    result.Min.x += x;
    result.Min.y += y;
    result.Max.x += x;
    result.Max.y += y;
    return result;
}

INTERNAL_FUNC bool begin_menubar(const ImRect& barRectangle) {
    // Safety check: ensure ImGui context is valid
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    if (!ctx) {
        return false;
    }

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems)
        return false;

    IM_ASSERT(!window->DC.MenuBarAppending);
    ImGui::BeginGroup();
    ImGui::PushID("##menubar");

    const ImVec2 padding = window->WindowPadding;

    ImRect bar_rect = barRectangle; // Don't add padding offset
    ImRect clip_rect(
        IM_ROUND(ImMax(window->Pos.x,
            bar_rect.Min.x + window->WindowBorderSize + window->Pos.x - 10.0f)),
        IM_ROUND(bar_rect.Min.y + window->WindowBorderSize + window->Pos.y),
        IM_ROUND(ImMax(bar_rect.Min.x + window->Pos.x,
            bar_rect.Max.x -
                ImMax(window->WindowRounding, window->WindowBorderSize))),
        IM_ROUND(bar_rect.Max.y + window->Pos.y));

    clip_rect.ClipWith(window->OuterRectClipped);
    ImGui::PushClipRect(clip_rect.Min, clip_rect.Max, false);

    window->DC.CursorPos = window->DC.CursorMaxPos =
        ImVec2(bar_rect.Min.x + window->Pos.x, bar_rect.Min.y + window->Pos.y);

    window->DC.LayoutType = ImGuiLayoutType_Horizontal;
    window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
    window->DC.MenuBarAppending = true;
    ImGui::AlignTextToFramePadding();
    return true;
}

INTERNAL_FUNC void end_menubar() {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;
    ImGuiContext& g = *GImGui;

    // Nav: When a move request within one of our child menu failed, capture the
    // request to navigate among our siblings.
    if (ImGui::NavMoveRequestButNoResultYet() &&
        (g.NavMoveDir == ImGuiDir_Left || g.NavMoveDir == ImGuiDir_Right) &&
        (g.NavWindow->Flags & ImGuiWindowFlags_ChildMenu)) {
        ImGuiWindow* nav_earliest_child = g.NavWindow;
        while (nav_earliest_child->ParentWindow &&
               (nav_earliest_child->ParentWindow->Flags &
                   ImGuiWindowFlags_ChildMenu))
            nav_earliest_child = nav_earliest_child->ParentWindow;
        if (nav_earliest_child->ParentWindow == window &&
            nav_earliest_child->DC.ParentLayoutType ==
                ImGuiLayoutType_Horizontal &&
            (g.NavMoveFlags & ImGuiNavMoveFlags_Forwarded) == 0) {
            const ImGuiNavLayer layer = ImGuiNavLayer_Menu;
            IM_ASSERT(window->DC.NavLayersActiveMaskNext & (1 << layer));
            ImGui::FocusWindow(window);
            ImGui::SetNavID(window->NavLastIds[layer],
                layer,
                0,
                window->NavRectRel[layer]);
            g.NavCursorVisible = false;
            g.NavHighlightItemUnderNav = g.NavMousePosDirty = true;
            ImGui::NavMoveRequestForward(g.NavMoveDir,
                g.NavMoveClipDir,
                g.NavMoveFlags,
                g.NavMoveScrollFlags);
        }
    }

    IM_ASSERT(window->DC.MenuBarAppending);
    ImGui::PopClipRect();
    ImGui::PopID();
    window->DC.MenuBarOffset.x = window->DC.CursorPos.x - window->Pos.x;
    g.GroupStack.back().EmitItem = false;
    ImGui::EndGroup();
    window->DC.LayoutType = ImGuiLayoutType_Vertical;
    window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
    window->DC.MenuBarAppending = false;
}

INTERNAL_FUNC void handle_titlebar_hover() {
    // Create invisible button for the drag area (like Walnut does)
    f32 button_area_width =
        3 * (26.0f + 2.0f) + 4.0f; // 3 buttons + spacing + margin
    f32 drag_zone_width =
        (state.titlebar_max.x - state.titlebar_min.x) - button_area_width;
    f32 drag_zone_height = state.titlebar_max.y - state.titlebar_min.y;

    // Ensure valid drag zone dimensions
    if (drag_zone_width <= 0 || drag_zone_height <= 0) {
        CORE_ERROR("Invalid drag zone dimensions: width=%.2f, height=%.2f",
                   drag_zone_width, drag_zone_height);
        state.is_titlebar_hovered = false;
        return;
    }

    // Position the invisible button to cover the draggable area
    ImGui::SetCursorScreenPos(state.titlebar_min);

    // Push flags to prevent this button from being a docking target
    ImGui::PushItemFlag(ImGuiItemFlags_NoTabStop, true);
    ImGui::InvisibleButton("##titleBarDragZone",
        ImVec2(drag_zone_width, drag_zone_height));
    ImGui::PopItemFlag();

    // Set hover state based on the button (like Walnut)
    state.is_titlebar_hovered = ImGui::IsItemHovered();

    // If menu is hovered, disable dragging (like Walnut does)
    if (state.is_menu_hovered) {
        state.is_titlebar_hovered = false;
    }

    // Handle double-click to maximize/restore (SDL will handle single-click
    // dragging)
    if (state.is_titlebar_hovered &&
        ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        if (platform_is_window_maximized()) {
            platform_restore_window();
        } else {
            platform_maximize_window();
        }
    }
}

b8 ui_is_titlebar_hovered() {
    return state.is_titlebar_hovered;
}

// Helper function to load UI images directly using image loader + renderer
INTERNAL_FUNC b8 load_ui_image(const char* image_name, UI_Image_Resource* image_resource) {
    if (!image_name || !image_resource) {
        CORE_ERROR("Invalid parameters for image loading");
        return false;
    }

    // Load and decode image data
    Image_Load_Result result = image_loader_load(image_name);
    if (!result.success) {
        CORE_ERROR("Failed to load image '%s': %s", image_name, result.error_message);
        return false;
    }

    // Create GPU resource through renderer using pre-allocated resource
    b8 success = renderer_create_ui_image(
        result.width,
        result.height,
        result.pixel_data,
        result.pixel_data_size,
        image_resource);

    // Clean up decoded pixel data (renderer has copied it)
    stbi_image_free(result.pixel_data);

    if (!success) {
        CORE_ERROR("Failed to create GPU image resource for '%s'", image_name);
        return false;
    }

    CORE_DEBUG("Successfully loaded image resource: %s (%ux%u)",
        image_name, result.width, result.height);
    return true;
}
