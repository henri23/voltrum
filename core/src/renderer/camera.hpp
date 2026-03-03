#pragma once

#include "defines.hpp"
#include "math/math_types.hpp"

enum class Camera_Mode : u8
{
    _2D,
    _3D
};

struct Camera
{
    vec3 position;
    vec3 rotation;
    f32  zoom;

    Camera_Mode mode;
    b8          is_dirty;
    u8          _padding[2];

    mat4 view;
};

Camera camera_create();
void   camera_reset(Camera *camera);

vec3 camera_position_get(const Camera *camera);
void camera_position_set(Camera *camera, vec3 position);

vec3 camera_rotation_euler_get(const Camera *c);
void camera_rotation_euler_set(Camera *c, vec3 rotation);
mat4 camera_view_get(Camera *camera);

vec3 camera_forward(Camera *c);
vec3 camera_backward(Camera *c);
vec3 camera_left(Camera *c);
vec3 camera_right(Camera *c);
vec3 camera_up(Camera *c);
vec3 camera_down(Camera *c);

void camera_move_forward(Camera *c, f32 amount);
void camera_move_backward(Camera *c, f32 amount);
void camera_move_left(Camera *c, f32 amount);
void camera_move_right(Camera *c, f32 amount);
void camera_move_up(Camera *c, f32 amount);
void camera_move_down(Camera *c, f32 amount);

void camera_yaw(Camera *c, f32 amount);
void camera_pitch(Camera *c, f32 amount);
