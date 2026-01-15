#include "core/absolute_clock.hpp"
#include "platform/platform.hpp"

void absolute_clock_start(Absolute_Clock *clock) {
    clock->start_time = platform_get_absolute_time();
    clock->elapsed_time = 0;
}

void absolute_clock_stop(Absolute_Clock *clock) {
    clock->start_time = 0;
    // We may want to know the elapsed time after the clock is shutdown
}

void absolute_clock_update(Absolute_Clock *clock) {
    if (clock->start_time != 0)
        clock->elapsed_time = platform_get_absolute_time() - clock->start_time;
}
