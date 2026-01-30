#include "input.hpp"
#include "core/asserts.hpp"
#include "core/logger.hpp"
#include "memory/memory.hpp"

internal_var Input_State *state_ptr = nullptr;

Input_State *
input_init(Arena *allocator)
{
    CORE_DEBUG("Initializing input system...");

    RUNTIME_ASSERT(state_ptr == nullptr);

    state_ptr                 = push_struct(allocator, Input_State);
    state_ptr->is_initialized = true;

    CORE_INFO("Input system initialized successfully");

    return state_ptr;
}

void
input_update()
{
    ENSURE(state_ptr);

    // Copy current state to previous state for edge detection
    memory_copy(state_ptr->keys_prev, state_ptr->keys, sizeof(state_ptr->keys));
    memory_copy(state_ptr->mouse_buttons_prev,
                state_ptr->mouse_buttons,
                sizeof(state_ptr->mouse_buttons));

    // Update mouse delta
    state_ptr->mouse_delta_x = state_ptr->mouse_x - state_ptr->mouse_prev_x;
    state_ptr->mouse_delta_y = state_ptr->mouse_y - state_ptr->mouse_prev_y;

    // Store previous mouse position
    state_ptr->mouse_prev_x = state_ptr->mouse_x;
    state_ptr->mouse_prev_y = state_ptr->mouse_y;

    // Reset wheel delta (it's only valid for one frame)
    state_ptr->mouse_wheel_delta_x = 0.0f;
    state_ptr->mouse_wheel_delta_y = 0.0f;
}

void
input_process_key(Key_Code key_code, b8 pressed)
{
    ENSURE(state_ptr);
    RUNTIME_ASSERT((u32)key_code <= (u32)Key_Code::MAX_KEYS);

    state_ptr->keys[(u32)key_code] = pressed;
}

void
input_process_mouse_button(Mouse_Button button, b8 pressed)
{
    ENSURE(state_ptr);
    RUNTIME_ASSERT((u32)button <= (u32)Mouse_Button::MAX_BUTTONS);

    state_ptr->mouse_buttons[(u8)button] = pressed;
}

void
input_process_mouse_move(s32 x, s32 y)
{
    ENSURE(state_ptr);

    state_ptr->mouse_x = x;
    state_ptr->mouse_y = y;
}

void
input_process_mouse_wheel(f32 delta_x, f32 delta_y)
{
    ENSURE(state_ptr);

    state_ptr->mouse_wheel_delta_x = delta_x;
    state_ptr->mouse_wheel_delta_y = delta_y;
}

// Utility functions for common input checks
b8
input_is_key_pressed(Key_Code key_code)
{
    ENSURE(state_ptr);
    RUNTIME_ASSERT((u32)key_code <= (u32)Key_Code::MAX_KEYS);

    return state_ptr->keys[(u32)key_code];
}

b8
input_is_key_released(Key_Code key_code)
{
    return !input_is_key_pressed(key_code);
}

b8
input_was_key_pressed(Key_Code key_code)
{
    ENSURE(state_ptr);
    RUNTIME_ASSERT((u32)key_code <= (u32)Key_Code::MAX_KEYS);

    return state_ptr->keys[(u32)key_code] &&
           !state_ptr->keys_prev[(u32)key_code];
}

b8
input_was_key_released(Key_Code key_code)
{
    ENSURE(state_ptr);
    RUNTIME_ASSERT((u32)key_code <= (u32)Key_Code::MAX_KEYS);

    return !state_ptr->keys[(u32)key_code] &&
           state_ptr->keys_prev[(u32)key_code];
}

b8
input_is_mouse_button_pressed(Mouse_Button button)
{
    ENSURE(state_ptr);
    RUNTIME_ASSERT((u32)button <= (u32)Mouse_Button::MAX_BUTTONS);

    return state_ptr->mouse_buttons[(u8)button];
}

b8
input_is_mouse_button_released(Mouse_Button button)
{
    return !input_is_mouse_button_pressed(button);
}

b8
input_was_mouse_button_pressed(Mouse_Button button)
{
    ENSURE(state_ptr);
    RUNTIME_ASSERT((u32)button <= (u32)Mouse_Button::MAX_BUTTONS);

    return state_ptr->mouse_buttons[(u8)button] &&
           !state_ptr->mouse_buttons_prev[(u8)button];
}

b8
input_was_mouse_button_released(Mouse_Button button)
{
    ENSURE(state_ptr);
    RUNTIME_ASSERT((u32)button <= (u32)Mouse_Button::MAX_BUTTONS);

    return !state_ptr->mouse_buttons[(u8)button] &&
           state_ptr->mouse_buttons_prev[(u8)button];
}
