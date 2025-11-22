#pragma once

#include "defines.hpp"
#include "utils/enum.hpp"

// Engine keyboard key codes - based on USB HID standard (same as SDL3)
enum class Key_Code : u32 {
    UNKNOWN = 0,

    // Letters (A-Z)
    A = 4,
    B = 5,
    C = 6,
    D = 7,
    E = 8,
    F = 9,
    G = 10,
    H = 11,
    I = 12,
    J = 13,
    K = 14,
    L = 15,
    M = 16,
    N = 17,
    O = 18,
    P = 19,
    Q = 20,
    R = 21,
    S = 22,
    T = 23,
    U = 24,
    V = 25,
    W = 26,
    X = 27,
    Y = 28,
    Z = 29,

    // Numbers (1-9, 0)
    KEY_1 = 30,
    KEY_2 = 31,
    KEY_3 = 32,
    KEY_4 = 33,
    KEY_5 = 34,
    KEY_6 = 35,
    KEY_7 = 36,
    KEY_8 = 37,
    KEY_9 = 38,
    KEY_0 = 39,

    // Common keys
    RETURN = 40,
    ESCAPE = 41,
    BACKSPACE = 42,
    TAB = 43,
    SPACE = 44,
    MINUS = 45,
    EQUALS = 46,
    LEFTBRACKET = 47,
    RIGHTBRACKET = 48,
    BACKSLASH = 49,
    SEMICOLON = 51,
    APOSTROPHE = 52,
    GRAVE = 53,
    COMMA = 54,
    PERIOD = 55,
    SLASH = 56,
    CAPSLOCK = 57,

    // Function keys
    F1 = 58,
    F2 = 59,
    F3 = 60,
    F4 = 61,
    F5 = 62,
    F6 = 63,
    F7 = 64,
    F8 = 65,
    F9 = 66,
    F10 = 67,
    F11 = 68,
    F12 = 69,

    // Navigation keys
    PRINTSCREEN = 70,
    SCROLLLOCK = 71,
    PAUSE = 72,
    INSERT = 73,
    HOME = 74,
    PAGEUP = 75,
    DELETE = 76,
    END = 77,
    PAGEDOWN = 78,
    RIGHT = 79,
    LEFT = 80,
    DOWN = 81,
    UP = 82,

    // Numpad
    NUMLOCKCLEAR = 83,
    KP_DIVIDE = 84,
    KP_MULTIPLY = 85,
    KP_MINUS = 86,
    KP_PLUS = 87,
    KP_ENTER = 88,
    KP_1 = 89,
    KP_2 = 90,
    KP_3 = 91,
    KP_4 = 92,
    KP_5 = 93,
    KP_6 = 94,
    KP_7 = 95,
    KP_8 = 96,
    KP_9 = 97,
    KP_0 = 98,
    KP_PERIOD = 99,

    // Modifier keys
    LCTRL = 224,
    LSHIFT = 225,
    LALT = 226,
    LGUI = 227, // Left Windows/Cmd key
    RCTRL = 228,
    RSHIFT = 229,
    RALT = 230,
    RGUI = 231, // Right Windows/Cmd key

    // Common shortcuts for EDA tools
    CTRL = LCTRL, // Alias for easier use
    SHIFT = LSHIFT,
    ALT = LALT,
    CMD = LGUI,   // macOS Command key
    SUPER = LGUI, // Linux Super key

    MAX_KEYS = 512
};

// Engine mouse button codes
enum class Mouse_Button : u8 {
    UNKNOWN = 0,
    LEFT = 1,
    MIDDLE = 2,
    RIGHT = 3,
    X1 = 4, // Extra button 1 (back)
    X2 = 5, // Extra button 2 (forward)

    MAX_BUTTONS = 8
};

enum class Key_Modifiers : u32 {
    NONE = 0,
    SHIFT = 1 << 0,
    CTRL = 1 << 1,
    ALT = 1 << 2,
};

ENABLE_BITMASK(Key_Modifiers);

// Utility functions to convert between engine codes and platform codes
u32 key_code_to_platform(Key_Code key);
Key_Code platform_to_key_code(u32 platform_key);

u8 mouse_button_to_platform(Mouse_Button button);
Mouse_Button platform_to_mouse_button(u8 platform_button);
