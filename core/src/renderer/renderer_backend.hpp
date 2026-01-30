#pragma once

#include "renderer/renderer_types.hpp"

b8 renderer_backend_initialize(Renderer_Backend_Type  type,
                               Arena                 *allocator,
                               Renderer_Backend      *out_backend);
