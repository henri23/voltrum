#include "events.hpp"
#include "core/asserts.hpp"
#include "core/logger.hpp"

constexpr u32 DEFAULT_EVENT_CALLBACK_COUNT = 4;

internal_var Event_State *state_ptr = nullptr;

Event_Queue *
event_queue_create(Arena *allocator, u64 capacity)
{
    Event_Queue *eq = push_struct(allocator, Event_Queue);
    eq->queue.init(allocator, capacity);
    return eq;
}

void
event_queue_produce(Event_Queue  *eq,
                    const Event  &event)
{
    RUNTIME_ASSERT(eq != nullptr);
    eq->queue.enqueue(event);
}

INTERNAL_FUNC b8
event_queue_consume(Event_Queue *eq, Event *out)
{
    RUNTIME_ASSERT(eq != nullptr);
    return eq->queue.dequeue(out);
}

void
event_queue_reset(Event_Queue *eq)
{
    RUNTIME_ASSERT(eq != nullptr);
    eq->queue.reset();
}

Event_State *
events_init(Arena *allocator)
{
    constexpr u32 max_event_count = (u32)Event_Type::MAX_EVENTS;

    CORE_DEBUG("Initializing event system...");

    Event_State *state = push_struct(allocator, Event_State);

    // Initialize callback arrays for each event type
    for (u32 i = 0; i < max_event_count; ++i)
    {
        // Arena should guarantee this memory to be 0, but explicitly set is to
        // 0 for safety
        state->callback_buckets->count = 0;

        state->callback_buckets[i].listeners =
            push_array(allocator, Event_Listener, DEFAULT_EVENT_CALLBACK_COUNT);
    }

    CORE_INFO("Event system initialized successfully");

    state_ptr = state;

    return state;
}

void
events_register_callback(Event_Type         event_type,
                         PFN_event_callback callback,
                         Event_Priority     priority)
{
    ENSURE(state_ptr);

    if ((u32)event_type >= (u32)Event_Type::MAX_EVENTS)
    {
        CORE_ERROR("Invalid event type: %d", (int)event_type);
        return;
    }

    Event_Callback_Bucket *bucket =
        &state_ptr->callback_buckets[(u32)event_type];

    RUNTIME_ASSERT_MSG(bucket->count < DEFAULT_EVENT_CALLBACK_COUNT,
                       "events_register_callback - Cannot publish event "
                       "because we are exceeding the limit of callbacks");

    // Create new callback entry
    Event_Listener new_listener;
    new_listener.callback = callback;
    new_listener.listener = nullptr;
    new_listener.priority = priority;

    // Insert at the first empty position
    // NOTE: handle the priority when dispatching the event
    u32 insertion_index = bucket->count; // Assume we will be writing here

    for (u32 i = 0; i < DEFAULT_EVENT_CALLBACK_COUNT; ++i)
    {
        if (bucket->listeners[i].callback == nullptr)
        {
            insertion_index = i;
            break;
        }
    }

    bucket->listeners[insertion_index] = new_listener;

    ++bucket->count;

    CORE_DEBUG("Event callback registered for event type: %d with priority: %d",
               (int)event_type,
               (int)priority);
}

void
events_unregister_callback(Event_Type event_type, PFN_event_callback callback)
{
    ENSURE(state_ptr);

    RUNTIME_ASSERT((u32)event_type >= (u32)Event_Type::MAX_EVENTS);

    Event_Listener *listeners = state_ptr->callback_buckets->listeners;

    for (u32 i = 0; i < DEFAULT_EVENT_CALLBACK_COUNT; ++i)
    {
        if (listeners[i].callback == callback)
        {
            listeners[i].callback = nullptr;
            listeners[i].listener = nullptr;

            CORE_DEBUG("Event callback unregistered for event type: %d",
                       (int)event_type);
            return;
        }
    }

    CORE_WARN("Callback not found for unregistration, event type: %d",
              (int)event_type);
}

void
event_queue_flush(Event_Queue *eq)
{
    ENSURE(state_ptr);
    RUNTIME_ASSERT(eq != nullptr);

    Event event;
    while (event_queue_consume(eq, &event))
    {
        if ((u32)event.type >= (u32)Event_Type::MAX_EVENTS)
        {
            CORE_ERROR("Invalid event type in flush: %d", (int)event.type);
            continue;
        }

        Event_Callback_Bucket *bucket =
            &state_ptr->callback_buckets[(u32)event.type];

        // Dispatch in priority order (lower enum value = higher priority)
        b8 consumed = false;

        for (u32 p = (u32)Event_Priority::HIGHEST;
             p <= (u32)Event_Priority::LOWEST && !consumed;
             ++p)
        {
            for (u32 i = 0; i < bucket->count && !consumed; ++i)
            {
                Event_Listener *listener = &bucket->listeners[i];

                if (listener->callback == nullptr)
                    continue;

                if ((u32)listener->priority != p)
                    continue;

                consumed = listener->callback(&event);
            }
        }
    }
}
