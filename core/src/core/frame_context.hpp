#pragma once

#include "data_structures/ring_queue.hpp"
#include "events/events.hpp"
#include "memory/arena.hpp"

struct Frame_Context
{
    Arena             *arena;
    Ring_Queue<Event> *event_queue;
    f32                delta_t;
};
