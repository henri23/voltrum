#pragma once

#include "math_types.hpp"

// Zero position, identity rotation, unit scale
VOLTRUM_API Transform transform_identity();

VOLTRUM_API Transform transform_create(vec3 position,
                                       quat rotation,
                                       vec3 scale);

// Individual field setters (mark dirty)
VOLTRUM_API void transform_position_set(Transform *t, vec3 position);
VOLTRUM_API void transform_rotation_set(Transform *t, quat rotation);
VOLTRUM_API void transform_scale_set(Transform *t, vec3 scale);

// Compose operations (accumulate onto existing values)
VOLTRUM_API void transform_translate(Transform *t, vec3 offset);
VOLTRUM_API void transform_rotate(Transform *t, quat rotation);
VOLTRUM_API void transform_scale_by(Transform *t, vec3 scale);

// Returns the cached local matrix, recomputing only if dirty
VOLTRUM_API mat4 transform_get_local(Transform *t);

// VOLTRUM_API Transform *transform_get_parent(Transform *t);
// VOLTRUM_API void       transform_set_parent(Transform *t, Transform *parent);
// VOLTRUM_API mat4       transform_get_world(Transform *t);
