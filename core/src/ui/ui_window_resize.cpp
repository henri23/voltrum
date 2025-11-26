#include "ui_window_resize.hpp"

#include "core/logger.hpp"
#include "platform/platform.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

// Resize constants
const f32 RESIZE_BORDER_SIZE = 4.0f; // Size of resize border in pixels (reduced for less sensitivity)
const s32 MIN_WINDOW_WIDTH = 400;
const s32 MIN_WINDOW_HEIGHT = 300;

// Resize directions
enum Resize_Direction : u32 {
    RESIZE_NONE = 0,
    RESIZE_LEFT = 1 << 0,
    RESIZE_RIGHT = 1 << 1,
    RESIZE_TOP = 1 << 2,
    RESIZE_BOTTOM = 1 << 3,
    RESIZE_TOP_LEFT = RESIZE_TOP | RESIZE_LEFT,
    RESIZE_TOP_RIGHT = RESIZE_TOP | RESIZE_RIGHT,
    RESIZE_BOTTOM_LEFT = RESIZE_BOTTOM | RESIZE_LEFT,
    RESIZE_BOTTOM_RIGHT = RESIZE_BOTTOM | RESIZE_RIGHT
};

// Internal resize state
struct Window_Resize_State {
    b8 is_initialized;
    b8 is_resizing;
    Resize_Direction resize_direction;
    ImVec2 initial_mouse_pos;
    s32 initial_window_x, initial_window_y;
    s32 initial_window_width, initial_window_height;
};

internal_var Window_Resize_State state = {};

// Internal functions
INTERNAL_FUNC Resize_Direction get_resize_direction_from_mouse(ImVec2 mouse_pos, ImVec2 window_pos, ImVec2 window_size);
INTERNAL_FUNC ImGuiMouseCursor get_cursor_for_resize_direction(Resize_Direction direction);
INTERNAL_FUNC void perform_window_resize(ImVec2 current_mouse_pos);

b8 ui_window_resize_initialize() {
    if (state.is_initialized) {
        CORE_WARN("Window resize handler already initialized");
        return true;
    }

    state.is_initialized = true;
    state.is_resizing = false;
    state.resize_direction = RESIZE_NONE;

    CORE_INFO("Window resize handler initialized successfully");
    return true;
}

void ui_window_resize_shutdown() {
    if (!state.is_initialized) {
        CORE_WARN("Window resize handler not initialized");
        return;
    }

    state.is_initialized = false;
    state.is_resizing = false;
    state.resize_direction = RESIZE_NONE;

}

void ui_window_resize_handle() {
    if (!state.is_initialized) {
        return;
    }

    // Don't resize if window is maximized
    if (platform_is_window_maximized()) {
        state.is_resizing = false;
        return;
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 window_pos = viewport->Pos;
    ImVec2 window_size = viewport->Size;
    ImVec2 mouse_pos = ImGui::GetMousePos();

    if (!state.is_resizing) {
        // Check if we should start resizing
        Resize_Direction direction = get_resize_direction_from_mouse(mouse_pos, window_pos, window_size);

        // Set cursor based on resize direction
        ImGuiMouseCursor cursor = get_cursor_for_resize_direction(direction);
        if (cursor != ImGuiMouseCursor_Arrow) {
            ImGui::SetMouseCursor(cursor);
        }

        if (direction != RESIZE_NONE && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            // Start resizing
            state.is_resizing = true;
            state.resize_direction = direction;
            state.initial_mouse_pos = mouse_pos;

            platform_get_window_position(&state.initial_window_x, &state.initial_window_y);
            platform_get_window_size(&state.initial_window_width, &state.initial_window_height);
        }
    } else {
        // We are currently resizing
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            perform_window_resize(mouse_pos);
        } else {
            // Stop resizing when mouse is released
            state.is_resizing = false;
            state.resize_direction = RESIZE_NONE;
        }
    }
}

INTERNAL_FUNC Resize_Direction get_resize_direction_from_mouse(ImVec2 mouse_pos, ImVec2 window_pos, ImVec2 window_size) {
    Resize_Direction direction = RESIZE_NONE;

    // Check if mouse is near window edges
    b8 near_left = mouse_pos.x >= window_pos.x && mouse_pos.x <= window_pos.x + RESIZE_BORDER_SIZE;
    b8 near_right = mouse_pos.x >= window_pos.x + window_size.x - RESIZE_BORDER_SIZE && mouse_pos.x <= window_pos.x + window_size.x;
    b8 near_top = mouse_pos.y >= window_pos.y && mouse_pos.y <= window_pos.y + RESIZE_BORDER_SIZE;
    b8 near_bottom = mouse_pos.y >= window_pos.y + window_size.y - RESIZE_BORDER_SIZE && mouse_pos.y <= window_pos.y + window_size.y;

    // Make sure mouse is actually within or very close to window bounds
    b8 within_horizontal = mouse_pos.x >= window_pos.x - RESIZE_BORDER_SIZE && mouse_pos.x <= window_pos.x + window_size.x + RESIZE_BORDER_SIZE;
    b8 within_vertical = mouse_pos.y >= window_pos.y - RESIZE_BORDER_SIZE && mouse_pos.y <= window_pos.y + window_size.y + RESIZE_BORDER_SIZE;

    if (!within_horizontal || !within_vertical) {
        return RESIZE_NONE;
    }

    // Determine resize direction
    u32 dir_flags = 0;
    if (near_left) dir_flags |= RESIZE_LEFT;
    if (near_right) dir_flags |= RESIZE_RIGHT;
    if (near_top) dir_flags |= RESIZE_TOP;
    if (near_bottom) dir_flags |= RESIZE_BOTTOM;

    return (Resize_Direction)dir_flags;
}

INTERNAL_FUNC ImGuiMouseCursor get_cursor_for_resize_direction(Resize_Direction direction) {
    switch (direction) {
        case RESIZE_LEFT:
        case RESIZE_RIGHT:
            return ImGuiMouseCursor_ResizeEW;
        case RESIZE_TOP:
        case RESIZE_BOTTOM:
            return ImGuiMouseCursor_ResizeNS;
        case RESIZE_TOP_LEFT:
        case RESIZE_BOTTOM_RIGHT:
            return ImGuiMouseCursor_ResizeNWSE;
        case RESIZE_TOP_RIGHT:
        case RESIZE_BOTTOM_LEFT:
            return ImGuiMouseCursor_ResizeNESW;
        default:
            return ImGuiMouseCursor_Arrow;
    }
}

INTERNAL_FUNC void perform_window_resize(ImVec2 current_mouse_pos) {
    ImVec2 mouse_delta = ImVec2(current_mouse_pos.x - state.initial_mouse_pos.x,
                               current_mouse_pos.y - state.initial_mouse_pos.y);

    s32 new_x = state.initial_window_x;
    s32 new_y = state.initial_window_y;
    s32 new_width = state.initial_window_width;
    s32 new_height = state.initial_window_height;

    // Handle horizontal resizing
    if ((u32)state.resize_direction & RESIZE_LEFT) {
        s32 width_change = (s32)mouse_delta.x;
        new_width = state.initial_window_width - width_change;
        new_x = state.initial_window_x + width_change;
    } else if ((u32)state.resize_direction & RESIZE_RIGHT) {
        new_width = state.initial_window_width + (s32)mouse_delta.x;
    }

    // Handle vertical resizing
    if ((u32)state.resize_direction & RESIZE_TOP) {
        s32 height_change = (s32)mouse_delta.y;
        new_height = state.initial_window_height - height_change;
        new_y = state.initial_window_y + height_change;
    } else if ((u32)state.resize_direction & RESIZE_BOTTOM) {
        new_height = state.initial_window_height + (s32)mouse_delta.y;
    }

    // Enforce minimum window size
    if (new_width < MIN_WINDOW_WIDTH) {
        if ((u32)state.resize_direction & RESIZE_LEFT) {
            new_x = state.initial_window_x + state.initial_window_width - MIN_WINDOW_WIDTH;
        }
        new_width = MIN_WINDOW_WIDTH;
    }

    if (new_height < MIN_WINDOW_HEIGHT) {
        if ((u32)state.resize_direction & RESIZE_TOP) {
            new_y = state.initial_window_y + state.initial_window_height - MIN_WINDOW_HEIGHT;
        }
        new_height = MIN_WINDOW_HEIGHT;
    }

    // Get current window position/size to avoid unnecessary updates
    s32 current_x, current_y, current_width, current_height;
    platform_get_window_position(&current_x, &current_y);
    platform_get_window_size(&current_width, &current_height);

    // Only update if there's a meaningful change (reduce jitter)
    if (new_x != current_x || new_y != current_y) {
        platform_set_window_position(new_x, new_y);
    }
    if (new_width != current_width || new_height != current_height) {
        platform_set_window_size(new_width, new_height);
    }
}
