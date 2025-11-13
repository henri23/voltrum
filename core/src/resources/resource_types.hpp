#pragma once

#include "math/math_types.hpp"

// Interface
struct Texture {
    u32 id;
    u32 width;
    u32 height;
    u8 channel_count;
    b8 has_transparency;
    u32 generation;
    void* internal_data; // Graphics API specific data
};
