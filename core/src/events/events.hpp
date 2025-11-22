#pragma once

#include "defines.hpp"
#include "input/input_codes.hpp"

// Engine event types - platform agnostic
enum class Event_Type : u32 {
    NONE = 0,

    // Keyboard events
    KEY_PRESSED,
    KEY_RELEASED,

    // Mouse events
    MOUSE_BUTTON_PRESSED,
    MOUSE_BUTTON_RELEASED,
    MOUSE_MOVED,
    MOUSE_WHEEL_SCROLLED,

    // Window events
    WINDOW_CLOSED,
    WINDOW_RESIZED,
    WINDOW_MINIMIZED,
    WINDOW_MAXIMIZED,
    WINDOW_RESTORED,

    DEBUG0,
    DEBUG1,
    DEBUG2,
    DEBUG3,
    DEBUG4,

    MAX_EVENTS
};

// Engine event data structures
struct Key_Event {
    Key_Code key_code;
    b8 repeat;
    Key_Modifiers modifiers;
};

struct Mouse_Button_Event {
    Mouse_Button button;
    s32 x, y;
    Key_Modifiers modifiers;
};

struct Mouse_Move_Event {
    s32 x, y;
    s32 delta_x, delta_y;
};

struct Mouse_Wheel_Event {
    s32 x, y;
    f32 delta_x, delta_y;
};

struct Window_Resize_Event {
    u32 width, height;
};

// Engine event structure
struct Event {
    Event_Type type;

    union {
        Key_Event key;
        Mouse_Button_Event mouse_button;
        Mouse_Move_Event mouse_move;
        Mouse_Wheel_Event mouse_wheel;
        Window_Resize_Event window_resize;
    };
};

// Event callback type for consumers
using PFN_event_callback = b8 (*)(const Event*);

// Event callback priority levels (lower numbers = higher priority)
enum class Event_Priority {
    HIGHEST = 0, // Canvas/viewport operations
    HIGH = 1,    // Application logic
    NORMAL = 2,  // Default priority
    LOW = 3,     // Debug/logging callbacks
    LOWEST = 4 // UI (ImGui) - can consume without blocking important callbacks
};

// Event callback entry with priority and listener support
struct Event_Callback_Entry {
    PFN_event_callback callback;
    void* listener; // Always nullptr for now, future extensibility
    Event_Priority priority;
};

// Event system functions - following your top-down pattern
void events_initialize();
void events_shutdown();

// Register event callback for specific event type with priority (called by
// application)
VOLTRUM_API void events_register_callback(Event_Type event_type,
    PFN_event_callback callback,
    Event_Priority priority = Event_Priority::NORMAL);

// Unregister event callback for specific event type
VOLTRUM_API void events_unregister_callback(Event_Type event_type,
    PFN_event_callback callback);

// Called by platform layer to dispatch events
void events_dispatch(const Event* event);
