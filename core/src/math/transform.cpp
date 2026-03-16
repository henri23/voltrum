#include "transform.hpp"
#include "core/asserts.hpp"
#include "math/math.hpp"

Transform
transform_identity()
{
    Transform t;

    t.position = vec3_zero();
    t.rotation = quat_identity();
    t.scale    = vec3_one();
    // t.parent   = nullptr;

    t.is_dirty     = false;
    t.local_matrix = mat4_identity();

    return t;
}

Transform
transform_create(vec3 position, quat rotation, vec3 scale)
{
    Transform t;

    t.position = position;
    t.rotation = rotation;
    t.scale    = scale;
    // t.parent   = nullptr;

    t.is_dirty     = true;
    t.local_matrix = mat4_identity();

    return t;
}

void
transform_position_set(Transform *t, vec3 position)
{
    ENSURE(t);
    t->position = position;
    t->is_dirty = true;
}

void
transform_rotation_set(Transform *t, quat rotation)
{
    ENSURE(t);
    t->rotation = rotation;
    t->is_dirty = true;
}

void
transform_scale_set(Transform *t, vec3 scale)
{
    ENSURE(t);
    t->scale    = scale;
    t->is_dirty = true;
}

void
transform_translate(Transform *t, vec3 offset)
{
    ENSURE(t);
    t->position = t->position + offset;
    t->is_dirty = true;
}

void
transform_rotate(Transform *t, quat rotation)
{
    ENSURE(t);
    t->rotation = quat_mul(t->rotation, rotation);
    t->is_dirty = true;
}

void
transform_scale_by(Transform *t, vec3 scale)
{
    ENSURE(t);
    t->scale    = t->scale * scale;
    t->is_dirty = true;
}

mat4
transform_get_local(Transform *t)
{
    ENSURE(t);

    if (!t->is_dirty)
        return t->local_matrix;

    mat4 tr = quat_to_mat4(t->rotation) * mat4_translation(t->position);
    tr      = mat4_scale(t->scale) * tr;

    t->local_matrix = tr;
    t->is_dirty     = false;

    return t->local_matrix;
}

// Multiplies parent transform matrix to the local transform
// mat4
// transform_get_world(Transform *t)
// {
//     ENSURE(t);
//
//     mat4 l = transform_get_local(t);
//     if (t->parent)
//         return l * transform_get_world(t->parent);
//
//     return l;
// }
