#include "camera.hpp"
#include "core/asserts.hpp"

#include "math/math.hpp"

Camera
camera_create()
{
    Camera c;
    camera_reset(&c);
    return c;
}

void
camera_reset(Camera *camera)
{
    ENSURE(camera);
    camera->rotation = vec3_zero();
    camera->position = vec3_zero();
    camera->is_dirty = false;
    camera->view     = mat4_identity();
}

vec3
camera_position_get(const Camera *camera)
{
    ENSURE(camera);
    return camera->position;
}

void
camera_position_set(Camera *camera, vec3 position)
{
    ENSURE(camera);
    camera->position = position;
    camera->is_dirty = true;
}

vec3
camera_rotation_euler_get(const Camera *camera)
{
    ENSURE(camera);
    return camera->rotation;
}

void
camera_rotation_euler_set(Camera *camera, vec3 rotation)
{
    ENSURE(camera);
    camera->rotation = rotation;
    camera->is_dirty = true;
}

mat4
camera_view_get(Camera *camera)
{
    ENSURE(camera);

    if (!camera->is_dirty)
        return camera->view;

    mat4 rotation_matrix = mat4_euler_xyz(camera->rotation.x,
                                          camera->rotation.y,
                                          camera->rotation.z);

    mat4 translation_matrix = mat4_translation(camera->position);

    camera->view = rotation_matrix * translation_matrix;
    camera->view = mat4_inv(camera->view);

    camera->is_dirty = false;

    return camera->view;
}

vec3
camera_forward(Camera *c)
{
    ENSURE(c);
    mat4 view = camera_view_get(c);
    return mat4_forward(view);
}

vec3
camera_backward(Camera *c)
{
    ENSURE(c);
    mat4 view = camera_view_get(c);
    return mat4_backward(view);
}

vec3
camera_left(Camera *c)
{
    ENSURE(c);
    mat4 view = camera_view_get(c);
    return mat4_left(view);
}

vec3
camera_right(Camera *c)
{
    ENSURE(c);
    mat4 view = camera_view_get(c);
    return mat4_right(view);
}

vec3
camera_up(Camera *c)
{
    ENSURE(c);
    mat4 view = camera_view_get(c);
    return mat4_up(view);
}

vec3
camera_down(Camera *c)
{
    ENSURE(c);
    mat4 view = camera_view_get(c);
    return mat4_down(view);
}

void
camera_move_forward(Camera *c, f32 amount)
{
    ENSURE(c);

    vec3 direction = camera_forward(c);
    direction      = vec3_scale(direction, amount);
    c->position    = c->position + direction;
    c->is_dirty    = true;
}

void
camera_move_backward(Camera *c, f32 amount)
{
    ENSURE(c);

    vec3 direction = camera_backward(c);
    direction      = vec3_scale(direction, amount);
    c->position    = c->position + direction;
    c->is_dirty    = true;
}

void
camera_move_left(Camera *c, f32 amount)
{
    ENSURE(c);

    vec3 direction = camera_left(c);
    direction      = vec3_scale(direction, amount);
    c->position    = c->position + direction;
    c->is_dirty    = true;
}

void
camera_move_right(Camera *c, f32 amount)
{
    ENSURE(c);

    vec3 direction = camera_right(c);
    direction      = vec3_scale(direction, amount);
    c->position    = c->position + direction;
    c->is_dirty    = true;
}

void
camera_move_up(Camera *c, f32 amount)
{
    ENSURE(c);

    vec3 direction = camera_up(c);
    direction      = vec3_scale(direction, amount);
    c->position    = c->position + direction;
    c->is_dirty    = true;
}

void
camera_move_down(Camera *c, f32 amount)
{
    ENSURE(c);

    vec3 direction = camera_down(c);
    direction      = vec3_scale(direction, amount);
    c->position    = c->position + direction;
    c->is_dirty    = true;
}

void
camera_yaw(Camera *c, f32 amount)
{
    ENSURE(c);

    c->rotation.y += amount;
    c->is_dirty = true;
}

void
camera_pitch(Camera *c, f32 amount)
{
    ENSURE(c);

    c->rotation.x += amount;

    static const f32 limit = 1.55224206f; // 89 degrees for gimbal lock prev.
    c->rotation.x          = CLAMP(c->rotation.x, -limit, limit);

    c->is_dirty = true;
}
