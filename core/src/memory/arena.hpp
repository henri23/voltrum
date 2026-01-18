#pragma once

#include "defines.hpp"

struct Arena {
    u64 total_size;
    u64 occupied_size;
    void *memory;
};
