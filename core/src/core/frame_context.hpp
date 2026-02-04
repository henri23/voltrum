#pragma once

#include "defines.hpp"
#include "memory/arena.hpp"

struct Event_Queue;

struct Frame_Context
{
    Arena       *frame_arena;
    Event_Queue *event_queue;
    f32          delta_t;
};
