#include "math.hpp"
#include "platform/platform.hpp"

#include <math.h>
#include <stdlib.h>

internal_var b8 rand_seeded = false;

f32 math_sin(f32 x) {
    return sinf(x);
}

f32 math_cos(f32 x) {
    return cosf(x);
}

f32 math_tan(f32 x) {
    return tanf(x);
}

f32 math_arccos(f32 x) {
    return acosf(x);
}

f32 math_sqrt(f32 x) {
    return sqrtf(x);
}

f32 math_abs_value(f32 x) {
    return fabsf(x);
}

s32 math_random_signed() {
    if (!rand_seeded) {
        srand((u32)platform_get_absolute_time()); // pseudo-random seed
        rand_seeded = true;
    }
    return rand();
}

f32 math_random_float() {
    return (f32)math_random_signed() / (f32)RAND_MAX;
}

s32 math_random_signed_in_range(s32 min, s32 max) {
    if (!rand_seeded) {
        srand((u32)platform_get_absolute_time()); // pseudo-random seed
        rand_seeded = true;
    }
    return (rand() % (max - min + 1) + min);
}

f32 math_random_float_in_range(f32 min, f32 max) {
    return min + ((f32)math_random_signed() / ((f32)RAND_MAX / (max - min)));
}
