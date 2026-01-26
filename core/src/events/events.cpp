#include "events.hpp"
#include "core/logger.hpp"
#include "memory/memory.hpp"

void
events_initialize() {
    CORE_DEBUG("Initializing event system...");

    if (event_state.is_initialized) {
        CORE_WARN("Event system already initialized");
        return;
    }

    // Zero out the state
    memory_zero(&event_state, sizeof(Event_State));

    // Initialize callback arrays for each event type
    for (u32 i = 0; i < (u32)Event_Type::MAX_EVENTS; ++i) {
        event_state.callbacks[i] = Auto_Array<Event_Callback_Entry>();
    }

    event_state.is_initialized = true;

    CORE_INFO("Event system initialized successfully");
}

void
events_shutdown() {
    CORE_DEBUG("Shutting down event system...");

    if (!event_state.is_initialized) {
        CORE_WARN("Event system not initialized");
        return;
    }

    // Clear all callback arrays
    for (u32 i = 0; i < (u32)Event_Type::MAX_EVENTS; ++i) {
        event_state.callbacks[i].clear();
    }

    event_state.is_initialized = false;

    CORE_DEBUG("Event system shut down");
}

void
events_register_callback(Event_Type event_type,
    PFN_event_callback callback,
    Event_Priority priority) {
    if (!event_state.is_initialized) {
        CORE_ERROR("Event system not initialized");
        return;
    }

    if ((u32)event_type >= (u32)Event_Type::MAX_EVENTS) {
        CORE_ERROR("Invalid event type: %d", (int)event_type);
        return;
    }

    Auto_Array<Event_Callback_Entry> &callbacks =
        event_state.callbacks[(u32)event_type];

    // Create new callback entry
    Event_Callback_Entry new_entry;
    new_entry.callback = callback;
    new_entry.listener = nullptr;
    new_entry.priority = priority;

    // Find insertion position to maintain ascending priority order (lower
    // priority number = higher priority)
    u32 insertion_index = 0;
    for (u32 i = 0; i < callbacks.length; ++i) {
        if ((u32)callbacks[i].priority > (u32)priority) {
            insertion_index = i;
            break;
        }
        insertion_index = i + 1;
    }

    // Insert at the correct position
    callbacks.insert(callbacks.data + insertion_index, new_entry);

    CORE_DEBUG(
        "Event callback registered for event type: %d with priority: %d at "
        "index: %d",
        (int)event_type,
        (int)priority,
        insertion_index);
}

void
events_unregister_callback(Event_Type event_type, PFN_event_callback callback) {
    if (!event_state.is_initialized) {
        CORE_ERROR("Event system not initialized");
        return;
    }

    if ((u32)event_type >= (u32)Event_Type::MAX_EVENTS) {
        CORE_ERROR("Invalid event type: %d", (int)event_type);
        return;
    }

    Auto_Array<Event_Callback_Entry> &callbacks =
        event_state.callbacks[(u32)event_type];
    for (u32 i = 0; i < callbacks.length; ++i) {
        if (callbacks[i].callback == callback) {
            callbacks.erase(&callbacks[i]);
            CORE_DEBUG("Event callback unregistered for event type: %d",
                (int)event_type);
            return;
        }
    }

    CORE_WARN("Callback not found for unregistration, event type: %d",
        (int)event_type);
}

void
events_dispatch(const Event *event) {
    if (!event_state.is_initialized) {
        return;
    }

    if ((u32)event->type >= (u32)Event_Type::MAX_EVENTS) {
        CORE_ERROR("Invalid event type in dispatch: %d", (int)event->type);
        return;
    }

    Auto_Array<Event_Callback_Entry> &callbacks =
        event_state.callbacks[(u32)event->type];

    // Dispatch to all callbacks for this event type in priority order
    // If any callback returns true (consumed), stop propagation
    for (u32 i = 0; i < callbacks.length; ++i) {
        if (callbacks[i].callback(event)) {
            // Event was consumed, stop propagation
            return;
        }
    }
}
