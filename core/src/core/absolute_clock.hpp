#pragma once
#include "defines.hpp"

struct Absolute_Clock {
    f64 start_time;
    f64 elapsed_time;
};

VOLTRUM_API void absolute_clock_start(Absolute_Clock *clock);
VOLTRUM_API void absolute_clock_stop(Absolute_Clock *clock);
VOLTRUM_API void absolute_clock_update(Absolute_Clock *clock);
