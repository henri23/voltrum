#include "input.hpp"
#include "core/logger.hpp"
#include "memory/memory.hpp"

// Internal input state
internal_var Input_State input_state;

void input_initialize() {
    CORE_DEBUG("Initializing input system...");

    if (input_state.is_initialized) {
        CORE_WARN("Input system already initialized");
        return;
    }

    // Zero out the state
    memory_zero(&input_state, sizeof(Input_State));
    input_state.is_initialized = true;

    CORE_INFO("Input system initialized successfully");
}

void input_shutdown() {
    CORE_DEBUG("Shutting down input system...");

    if (!input_state.is_initialized) {
        CORE_WARN("Input system not initialized");
        return;
    }

    input_state.is_initialized = false;
    CORE_DEBUG("Input system shut down");
}

void input_update() {
    if (!input_state.is_initialized) {
        return;
    }

    // Copy current state to previous state for edge detection
    memory_copy(input_state.keys_prev, input_state.keys, sizeof(input_state.keys));
    memory_copy(input_state.mouse_buttons_prev, input_state.mouse_buttons, sizeof(input_state.mouse_buttons));

    // Update mouse delta
    input_state.mouse_delta_x = input_state.mouse_x - input_state.mouse_prev_x;
    input_state.mouse_delta_y = input_state.mouse_y - input_state.mouse_prev_y;

    // Store previous mouse position
    input_state.mouse_prev_x = input_state.mouse_x;
    input_state.mouse_prev_y = input_state.mouse_y;

    // Reset wheel delta (it's only valid for one frame)
    input_state.mouse_wheel_delta_x = 0.0f;
    input_state.mouse_wheel_delta_y = 0.0f;
}

void input_process_key(Key_Code key_code, b8 pressed) {
    if (!input_state.is_initialized || (u32)key_code >= 512) {
        return;
    }

    input_state.keys[(u32)key_code] = pressed;
}

void input_process_mouse_button(Mouse_Button button, b8 pressed) {
    if (!input_state.is_initialized || (u8)button >= 8) {
        return;
    }

    input_state.mouse_buttons[(u8)button] = pressed;
}

void input_process_mouse_move(s32 x, s32 y) {
    if (!input_state.is_initialized) {
        return;
    }

    input_state.mouse_x = x;
    input_state.mouse_y = y;
}

void input_process_mouse_wheel(f32 delta_x, f32 delta_y) {
    if (!input_state.is_initialized) {
        return;
    }

    input_state.mouse_wheel_delta_x = delta_x;
    input_state.mouse_wheel_delta_y = delta_y;
}

Input_State* input_get_state() {
    if (!input_state.is_initialized) {
        return nullptr;
    }

    return &input_state;
}

// Utility functions for common input checks
b8 input_is_key_pressed(Key_Code key_code) {
    if (!input_state.is_initialized || (u32)key_code >= 512) {
        return false;
    }
    return input_state.keys[(u32)key_code];
}

b8 input_is_key_released(Key_Code key_code) {
    return !input_is_key_pressed(key_code);
}

b8 input_was_key_pressed(Key_Code key_code) {
    if (!input_state.is_initialized || (u32)key_code >= 512) {
        return false;
    }
    return input_state.keys[(u32)key_code] && !input_state.keys_prev[(u32)key_code];
}

b8 input_was_key_released(Key_Code key_code) {
    if (!input_state.is_initialized || (u32)key_code >= 512) {
        return false;
    }
    return !input_state.keys[(u32)key_code] && input_state.keys_prev[(u32)key_code];
}

b8 input_is_mouse_button_pressed(Mouse_Button button) {
    if (!input_state.is_initialized || (u8)button >= 8) {
        return false;
    }
    return input_state.mouse_buttons[(u8)button];
}

b8 input_is_mouse_button_released(Mouse_Button button) {
    return !input_is_mouse_button_pressed(button);
}

b8 input_was_mouse_button_pressed(Mouse_Button button) {
    if (!input_state.is_initialized || (u8)button >= 8) {
        return false;
    }
    return input_state.mouse_buttons[(u8)button] && !input_state.mouse_buttons_prev[(u8)button];
}

b8 input_was_mouse_button_released(Mouse_Button button) {
    if (!input_state.is_initialized || (u8)button >= 8) {
        return false;
    }
    return !input_state.mouse_buttons[(u8)button] && input_state.mouse_buttons_prev[(u8)button];
}