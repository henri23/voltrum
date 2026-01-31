#pragma once

#include "memory/arena.hpp"
#include "resources/resource_types.hpp"

b8 material_loader_load(Arena      *arena,
                        const char *name,
                        Resource   *out_resource);
