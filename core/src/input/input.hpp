#pragma once

#include "defines.hpp"
#include "input_codes.hpp"

// Input state - single struct approach, no getters/setters
struct Input_State {
    // Keyboard state
    b8 keys[512];      // Current frame key state
    b8 keys_prev[512]; // Previous frame for edge detection

    // Mouse state
    s32 mouse_x, mouse_y;             // Current mouse position
    s32 mouse_prev_x, mouse_prev_y;   // Previous mouse position
    s32 mouse_delta_x, mouse_delta_y; // Mouse delta this frame
    b8 mouse_buttons[8];              // Current frame button state
    b8 mouse_buttons_prev[8];         // Previous frame button state
    f32 mouse_wheel_delta_x, mouse_wheel_delta_y;

    b8 is_initialized;
};

// Input system functions - top-down approach
void input_initialize();
void input_shutdown();

// Called each frame by application to update input state
void input_update();

// Called by platform/event system to update input state
void input_process_key(Key_Code key_code, b8 pressed);
void input_process_mouse_button(Mouse_Button button, b8 pressed);
void input_process_mouse_move(s32 x, s32 y);
void input_process_mouse_wheel(f32 delta_x, f32 delta_y);

// Get the current input state (no individual getters)
VOLTRUM_API Input_State *input_get_state();

// Utility functions for common input checks
VOLTRUM_API b8 input_is_key_pressed(Key_Code key_code);
VOLTRUM_API b8 input_is_key_released(Key_Code key_code);
VOLTRUM_API b8 input_was_key_pressed(Key_Code key_code);
VOLTRUM_API b8 input_was_key_released(Key_Code key_code);

VOLTRUM_API b8 input_is_mouse_button_pressed(Mouse_Button button);
VOLTRUM_API b8 input_is_mouse_button_released(Mouse_Button button);
VOLTRUM_API b8 input_was_mouse_button_pressed(Mouse_Button button);
VOLTRUM_API b8 input_was_mouse_button_released(Mouse_Button button);