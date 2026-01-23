#pragma once

#include "defines.hpp"
#include "math_types.hpp"
#include "memory/memory.hpp"

// List of precomputed (in compile time) of math constants useful to speed up
// and simplify math calculations
namespace math {
    constexpr f32 PI = 3.14159265358979323846f;
    constexpr f32 PI_2 = 2 * PI;
    constexpr f32 PI_HALVES = PI * 0.5f;
    constexpr f32 PI_QUARTERS = PI * 0.25f;
    constexpr f32 PI_INV = 1.0f / PI;
    constexpr f32 PI_2_INV = 1.0f / PI_2;
    constexpr f32 SQRT2 = 1.41421356237309504880f;
    constexpr f32 SQRT3 = 1.73205080756887729352f;
    constexpr f32 SQRT2_INV = 0.70710678118654752440f;
    constexpr f32 SQRT3_INV = 0.57735026918962576450f;
    constexpr f32 DEG_RAD_FACTOR = PI / 180.0f;
    constexpr f32 RAD_DEG_FACTOR = 180.0f / PI;

    // Avoid the standard math.h INFINITY macro name to keep Intellisense happy
    constexpr f32 INFINITY_F = 1e30f;
    // The float epsilon is defined as the smallest number possible that, when
    // added to 1.0 yields a result different from 1.0, i.e. 1.0 + ESPILON
    // != 1.0
    constexpr f32 EPSILON =
        1.192092896e-07f; // Value of FLT_EPSILON from cfloat
} // namespace math
// Math functions
VOLTRUM_API f32 math_sin(f32 x);
VOLTRUM_API f32 math_cos(f32 x);
VOLTRUM_API f32 math_tan(f32 x);
VOLTRUM_API f32 math_arccos(f32 x);
VOLTRUM_API f32 math_sqrt(f32 x);
VOLTRUM_API f32 math_abs_value(f32 x);

FORCE_INLINE b8
math_is_power_of_2(u64 value) {
    return (value != 0) && ((value & (value - 1)) == 0);
};

FORCE_INLINE u64
math_next_power_of_2(u64 value) {
    // Fast power of two ceiling algorithm
    // Reference:
    // https://graphics.stanford.edu/%7Eseander/bithacks.html#RoundUpPowerOf2
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    value++;
    return value;
}

VOLTRUM_API s32 math_random_signed();
VOLTRUM_API f32 math_random_float();

VOLTRUM_API s32 math_random_signed_in_range(s32 min, s32 max);
VOLTRUM_API f32 math_random_float_in_range(f32 min, f32 max);

// Vector 2 functions and operators
FORCE_INLINE vec2
vec2_create(float x, float y) {
    vec2 result = {x, y};
    return result;
}

FORCE_INLINE vec2
vec2_zero() {
    vec2 result = {0.0f, 0.0f};
    return result;
}

FORCE_INLINE vec2
vec2_one() {
    vec2 result = {1.0f, 1.0f};
    return result;
}

FORCE_INLINE vec2
vec2_up() {
    vec2 result = {0.0f, 1.0f};
    return result;
}

FORCE_INLINE vec2
vec2_down() {
    vec2 result = {0.0f, -1.0f};
    return result;
}

FORCE_INLINE vec2
vec2_left() {
    vec2 result = {-1.0f, 0.0f};
    return result;
}

FORCE_INLINE vec2
vec2_right() {
    vec2 result = {2.0f, 0.0f};
    return result;
}

// Component-wise operations
INLINE_OPERATOR vec2
operator+(vec2 a, vec2 b) {
    vec2 result = {a.x + b.x, a.y + b.y};
    return result;
}

INLINE_OPERATOR vec2
operator-(vec2 a, vec2 b) {
    vec2 result = {a.x - b.x, a.y - b.y};
    return result;
}

INLINE_OPERATOR vec2
operator*(vec2 a, vec2 b) {
    vec2 result = {a.x * b.x, a.y * b.y};
    return result;
}

INLINE_OPERATOR vec2
operator/(vec2 a, vec2 b) {
    vec2 result = {a.x / b.x, a.y / b.y};
    return result;
}

FORCE_INLINE f32
vec2_dot(vec2 a, vec2 b) {
    return a.x * b.x + a.y * b.y;
}

FORCE_INLINE f32
vec2_length_squared(vec2 a) {
    return a.x * a.x + a.y * a.y;
}

FORCE_INLINE f32
vec2_length(vec2 a) {
    return math_sqrt(vec2_length_squared(a));
}

FORCE_INLINE void
vec2_norm(vec2 *a) {
    const f32 length = vec2_length(*a);
    a->x /= length;
    a->y /= length;
}

FORCE_INLINE vec2
vec2_norm_copy(vec2 a) {
    vec2_norm(&a);
    return a; // Returns a copy
}

FORCE_INLINE b8
vec2_are_equal(vec2 a, vec2 b, f32 tolerance) {
    if (math_abs_value(a.x - b.x) > tolerance)
        return false;

    if (math_abs_value(a.y - b.y) > tolerance)
        return false;

    return true;
}

FORCE_INLINE f32
vec2_distance(vec2 a, vec2 b) {
    vec2 distance_vector = {a.x - b.x, a.y - b.y};
    return vec2_length(distance_vector);
}

// Vector3 functions and operators
FORCE_INLINE vec3
vec3_create(float x, float y, float z) {
    vec3 result = {x, y, z};
    return result;
}

FORCE_INLINE vec3
vec3_zero() {
    vec3 result = {0.0f, 0.0f, 0.0f};
    return result;
}

FORCE_INLINE vec3
vec3_one() {
    vec3 result = {1.0f, 1.0f, 1.0f};
    return result;
}

FORCE_INLINE vec3
vec3_up() {
    vec3 result = {0.0f, 1.0f, 0.0f};
    return result;
}

FORCE_INLINE vec3
vec3_down() {
    vec3 result = {0.0f, -1.0f, 0.0f};
    return result;
}

FORCE_INLINE vec3
vec3_left() {
    vec3 result = {-1.0f, 0.0f, 0.0f};
    return result;
}

FORCE_INLINE vec3
vec3_right() {
    vec3 result = {1.0f, 0.0f, 0.0f};
    return result;
}

FORCE_INLINE vec3
vec3_forward() {
    vec3 result = {0.0f, 0.0f, 1.0f};
    return result;
}

FORCE_INLINE vec3
vec3_back() {
    vec3 result = {0.0f, 0.0f, -1.0f};
    return result;
}

// Component-wise operations
INLINE_OPERATOR vec3
operator+(vec3 a, vec3 b) {
    vec3 result = {a.x + b.x, a.y + b.y, a.z + b.z};
    return result;
}

INLINE_OPERATOR vec3
operator-(vec3 a, vec3 b) {
    vec3 result = {a.x - b.x, a.y - b.y, a.z - b.z};
    return result;
}

// Not really used a element-wise multiplication for vec3
INLINE_OPERATOR vec3
operator*(vec3 a, vec3 b) {
    vec3 result = {a.x * b.x, a.y * b.y, a.z * b.z};
    return result;
}

INLINE_OPERATOR vec3
operator/(vec3 a, vec3 b) {
    vec3 result = {a.x / b.x, a.y / b.y, a.z / b.z};
    return result;
}

FORCE_INLINE f32
vec3_dot(vec3 a, vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

FORCE_INLINE vec3
vec3_cross(vec3 a, vec3 b) {
    vec3 result = {a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y -
            a.y *
                b.x}; // Output is orthogonal to the plane described by a and b
    return result;
}

FORCE_INLINE f32
vec3_length_squared(vec3 a) {
    return a.x * a.x + a.y * a.y + a.z * a.z;
}

FORCE_INLINE f32
vec3_length(vec3 a) {
    return math_sqrt(vec3_length_squared(a));
}

FORCE_INLINE vec3
vec3_scale(vec3 a, f32 scalar) {
    vec3 result = {scalar * a.x, scalar * a.y, scalar * a.z};
    return result;
}

FORCE_INLINE void
vec3_norm(vec3 *a) {
    const f32 length = vec3_length(*a);
    a->x /= length;
    a->y /= length;
    a->z /= length;
}

FORCE_INLINE vec3
vec3_norm_copy(vec3 a) {
    vec3_norm(&a);
    return a; // Returns a copy
}

FORCE_INLINE b8
vec3_are_equal(vec3 a, vec3 b, f32 tolerance) {
    if (math_abs_value(a.x - b.x) > tolerance)
        return false;

    if (math_abs_value(a.y - b.y) > tolerance)
        return false;

    if (math_abs_value(a.z - b.z) > tolerance)
        return false;

    return true;
}

FORCE_INLINE f32
vec3_distance(vec3 a, vec3 b) {
    vec3 distance_vector = {a.x - b.x, a.y - b.y, a.z - b.z};

    return vec3_length(distance_vector);
}

// Vector4 functions and operators
FORCE_INLINE vec4
vec4_create(f32 x, f32 y, f32 z, f32 w) {
    vec4 out_vector;
    out_vector.x = x;
    out_vector.y = y;
    out_vector.z = z;
    out_vector.w = w;
    return out_vector;
}

// Convenience function to go from homogeneous vector to simple vec3
FORCE_INLINE vec3
vec3_from_vec4(vec4 a) {
    vec3 result = {a.x, a.y, a.z};
    return result;
}

FORCE_INLINE vec4
vec3_to_vec4(vec3 a, f32 w) {
    vec4 result = {a.x, a.y, a.z, w};
    return result;
}

FORCE_INLINE vec4
vec4_zero() {
    vec4 result = {0.0f, 0.0f, 0.0f, 0.0f};
    return result;
}

FORCE_INLINE vec4
vec4_one() {
    vec4 result = {1.0f, 1.0f, 1.0f, 1.0f};
    return result;
}

INLINE_OPERATOR vec4
operator+(vec4 a, vec4 b) {
    vec4 result = {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
    return result;
}

INLINE_OPERATOR vec4
operator-(vec4 a, vec4 b) {
    vec4 result = {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
    return result;
}

// Not really used a element-wise multiplication forvec4
INLINE_OPERATOR vec4
operator*(vec4 a, vec4 b) {
    vec4 result = {a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
    return result;
}

INLINE_OPERATOR vec4
operator/(vec4 a, vec4 b) {
    vec4 result = {a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w};
    return result;
}

FORCE_INLINE f32
vec4_length_squared(vec4 a) {
    return a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w;
}

FORCE_INLINE f32
vec4_length(vec4 a) {
    return math_sqrt(vec4_length_squared(a));
}

FORCE_INLINE void
vec4_norm(vec4 *a) {
    const f32 length = vec4_length(*a);
    a->x /= length;
    a->y /= length;
    a->z /= length;
    a->w /= length;
}

FORCE_INLINE vec4
vec4_norm_copy(vec4 a) {
    vec4_norm(&a);
    return a; // Returns a copy
}

FORCE_INLINE f32
vec4_dot_f32(f32 ax, f32 ay, f32 az, f32 aw, f32 bx, f32 by, f32 bz, f32 bw) {
    return ax * bx + ay * by + az * bz + aw * bw;
}

FORCE_INLINE mat4
mat4_identity() {
    mat4 out_matrix;
    memory_zero(&out_matrix, sizeof(f32) * 16);
    out_matrix.elements[0] = 1.0f;
    out_matrix.elements[5] = 1.0f;
    out_matrix.elements[10] = 1.0f;
    out_matrix.elements[15] = 1.0f;
    return out_matrix;
}

INLINE_OPERATOR mat4
operator*(mat4 m1, mat4 m2) {
    mat4 out_matrix = mat4_identity();

    const f32 *matrix1 = m1.elements;
    const f32 *matrix2 = m2.elements;

    f32 *dest_matrix = out_matrix.elements;
    for (u32 i = 0; i < 4; ++i) {
        for (u32 j = 0; j < 4; ++j) {
            *dest_matrix =
                matrix1[0] * matrix2[0 + j] + matrix1[1] * matrix2[4 + j] +
                matrix1[2] * matrix2[8 + j] + matrix1[3] * matrix2[12 + j];

            dest_matrix++;
        }
        // We skip 4 elements from the current position to go to the next row
        matrix1 += 4;
    }

    return out_matrix;
}

// Create and return an orthographic projection matrix given a frostum and the
// near and far clipping planes. For an orthographic projection, unlike presp.
// projection, the object retain their sized regardless of their distance from
// the camera.
FORCE_INLINE mat4
mat4_project_orthographic(f32 left,
    f32 right,
    f32 bottom,
    f32 top,
    f32 near_clip,
    f32 far_clip) {
    mat4 out_matrix = mat4_identity();

    f32 lr = 1.0f / (left - right);
    f32 bt = 1.0f / (bottom - top);
    f32 nf = 1.0f / (near_clip - far_clip);

    out_matrix.elements[0] = -2.0f * lr;
    out_matrix.elements[5] = -2.0f * bt;
    out_matrix.elements[10] = -2.0f * nf;

    out_matrix.elements[12] = (left + right) * lr;
    out_matrix.elements[13] = (top + bottom) * bt;
    out_matrix.elements[14] = (far_clip + near_clip) * nf;

    return out_matrix;
}

FORCE_INLINE mat4
mat4_project_perspective(f32 fov_radians,
    f32 aspect_ratio,
    f32 near_clip,
    f32 far_clip) {
    f32 half_tan_fov = math_tan(fov_radians * 0.5f);
    mat4 out_matrix;

    memory_zero(out_matrix.elements, sizeof(f32) * 16);

    out_matrix.elements[0] = 1.0f / (aspect_ratio * half_tan_fov);
    out_matrix.elements[5] = 1.0f / half_tan_fov;
    // Vulkan depth range [0, 1]
    out_matrix.elements[10] = far_clip / (near_clip - far_clip);
    out_matrix.elements[11] = -1.0f;
    out_matrix.elements[14] = (near_clip * far_clip) / (near_clip - far_clip);

    return out_matrix;
}

// Create and returns a look at matrix, or a matrix looking at target from the
// perspective of position
FORCE_INLINE mat4
mat4_look_at(vec3 position, vec3 target, vec3 up) {
    mat4 out_matrix;
    vec3 z_axis;

    z_axis.x = target.x - position.x;
    z_axis.y = target.y - position.y;
    z_axis.z = target.z - position.z;
    z_axis = vec3_norm_copy(z_axis);
    vec3 x_axis = vec3_norm_copy(vec3_cross(z_axis, up));
    vec3 y_axis = vec3_cross(x_axis, z_axis);

    out_matrix.elements[0] = x_axis.x;
    out_matrix.elements[1] = y_axis.x;
    out_matrix.elements[2] = -z_axis.x;
    out_matrix.elements[3] = 0;

    out_matrix.elements[4] = x_axis.y;
    out_matrix.elements[5] = y_axis.y;
    out_matrix.elements[6] = -z_axis.y;
    out_matrix.elements[7] = 0;

    out_matrix.elements[8] = x_axis.z;
    out_matrix.elements[9] = y_axis.z;
    out_matrix.elements[10] = -z_axis.z;
    out_matrix.elements[11] = 0;

    out_matrix.elements[12] = -vec3_dot(x_axis, position);
    out_matrix.elements[13] = -vec3_dot(y_axis, position);
    out_matrix.elements[14] = vec3_dot(z_axis, position);
    out_matrix.elements[15] = 1.0f;

    return out_matrix;
}

FORCE_INLINE mat4
mat4_transpose(mat4 matrix) {

    mat4 out_matrix = mat4_identity();

    out_matrix.elements[0] = matrix.elements[0];
    out_matrix.elements[1] = matrix.elements[4];
    out_matrix.elements[2] = matrix.elements[8];
    out_matrix.elements[3] = matrix.elements[12];

    out_matrix.elements[4] = matrix.elements[1];
    out_matrix.elements[5] = matrix.elements[5];
    out_matrix.elements[6] = matrix.elements[9];
    out_matrix.elements[7] = matrix.elements[13];

    out_matrix.elements[8] = matrix.elements[2];
    out_matrix.elements[9] = matrix.elements[6];
    out_matrix.elements[10] = matrix.elements[10];
    out_matrix.elements[11] = matrix.elements[14];

    out_matrix.elements[12] = matrix.elements[3];
    out_matrix.elements[13] = matrix.elements[7];
    out_matrix.elements[14] = matrix.elements[11];
    out_matrix.elements[15] = matrix.elements[15];

    return out_matrix;
}

FORCE_INLINE mat4
mat4_inv(mat4 matrix) {
    const f32 *m = matrix.elements;

    f32 t0 = m[10] * m[15];
    f32 t1 = m[14] * m[11];
    f32 t2 = m[6] * m[15];
    f32 t3 = m[14] * m[7];
    f32 t4 = m[6] * m[11];
    f32 t5 = m[10] * m[7];
    f32 t6 = m[2] * m[15];
    f32 t7 = m[14] * m[3];
    f32 t8 = m[2] * m[11];
    f32 t9 = m[10] * m[3];
    f32 t10 = m[2] * m[7];
    f32 t11 = m[6] * m[3];
    f32 t12 = m[8] * m[13];
    f32 t13 = m[12] * m[9];
    f32 t14 = m[4] * m[13];
    f32 t15 = m[12] * m[5];
    f32 t16 = m[4] * m[9];
    f32 t17 = m[8] * m[5];
    f32 t18 = m[0] * m[13];
    f32 t19 = m[12] * m[1];
    f32 t20 = m[0] * m[9];
    f32 t21 = m[8] * m[1];
    f32 t22 = m[0] * m[5];
    f32 t23 = m[4] * m[1];

    mat4 out_matrix;

    f32 *o = out_matrix.elements;

    o[0] = (t0 * m[5] + t3 * m[9] + t4 * m[13]) -
           (t1 * m[5] + t2 * m[9] + t5 * m[13]);

    o[1] = (t1 * m[1] + t6 * m[9] + t9 * m[13]) -
           (t0 * m[1] + t7 * m[9] + t8 * m[13]);

    o[2] = (t2 * m[1] + t7 * m[5] + t10 * m[13]) -
           (t3 * m[1] + t6 * m[5] + t11 * m[13]);

    o[3] = (t5 * m[1] + t8 * m[5] + t11 * m[9]) -
           (t4 * m[1] + t9 * m[5] + t10 * m[9]);

    f32 d = 1.0f / (m[0] * o[0] + m[4] * o[1] + m[8] * o[2] + m[12] * o[3]);

    o[0] = d * o[0];
    o[1] = d * o[1];
    o[2] = d * o[2];
    o[3] = d * o[3];

    o[4] = d * ((t1 * m[4] + t2 * m[8] + t5 * m[12]) -
                   (t0 * m[4] + t3 * m[8] + t4 * m[12]));

    o[5] = d * ((t0 * m[0] + t7 * m[8] + t8 * m[12]) -
                   (t1 * m[0] + t6 * m[8] + t9 * m[12]));

    o[6] = d * ((t3 * m[0] + t6 * m[4] + t11 * m[12]) -
                   (t2 * m[0] + t7 * m[4] + t10 * m[12]));

    o[7] = d * ((t4 * m[0] + t9 * m[4] + t10 * m[8]) -
                   (t5 * m[0] + t8 * m[4] + t11 * m[8]));

    o[8] = d * ((t12 * m[7] + t15 * m[11] + t16 * m[15]) -
                   (t13 * m[7] + t14 * m[11] + t17 * m[15]));

    o[9] = d * ((t13 * m[3] + t18 * m[11] + t21 * m[15]) -
                   (t12 * m[3] + t19 * m[11] + t20 * m[15]));

    o[10] = d * ((t14 * m[3] + t19 * m[7] + t22 * m[15]) -
                    (t15 * m[3] + t18 * m[7] + t23 * m[15]));

    o[11] = d * ((t17 * m[3] + t20 * m[7] + t23 * m[11]) -
                    (t16 * m[3] + t21 * m[7] + t22 * m[11]));

    o[12] = d * ((t14 * m[10] + t17 * m[14] + t13 * m[6]) -
                    (t16 * m[14] + t12 * m[6] + t15 * m[10]));

    o[13] = d * ((t20 * m[14] + t12 * m[2] + t19 * m[10]) -
                    (t18 * m[10] + t21 * m[14] + t13 * m[2]));

    o[14] = d * ((t18 * m[6] + t23 * m[14] + t15 * m[2]) -
                    (t22 * m[14] + t14 * m[2] + t19 * m[6]));

    o[15] = d * ((t22 * m[10] + t16 * m[2] + t21 * m[6]) -
                    (t20 * m[6] + t23 * m[10] + t17 * m[2]));

    return out_matrix;
}

// 4D matrix that represents translation opertaion for homogeneous coordinates
FORCE_INLINE mat4
mat4_translation(vec3 position) {
    mat4 out_matrix = mat4_identity();

    out_matrix.elements[12] = position.x;
    out_matrix.elements[13] = position.y;
    out_matrix.elements[14] = position.z;

    return out_matrix;
}

FORCE_INLINE mat4
mat4_scale(vec3 scale) {
    mat4 out_matrix = mat4_identity();

    out_matrix.elements[0] = scale.x;
    out_matrix.elements[5] = scale.y;
    out_matrix.elements[10] = scale.z;

    return out_matrix;
}

FORCE_INLINE mat4
mat4_euler_x(f32 yaw_radians) {
    mat4 out_matrix = mat4_identity();

    f32 c = math_cos(yaw_radians);
    f32 s = math_sin(yaw_radians);

    out_matrix.elements[5] = c;
    out_matrix.elements[6] = s;
    out_matrix.elements[9] = -s;
    out_matrix.elements[10] = c;

    return out_matrix;
}

FORCE_INLINE mat4
mat4_euler_y(f32 pitch_radians) {
    mat4 out_matrix = mat4_identity();

    f32 c = math_cos(pitch_radians);
    f32 s = math_sin(pitch_radians);

    out_matrix.elements[0] = c;
    out_matrix.elements[2] = -s;
    out_matrix.elements[8] = s;
    out_matrix.elements[10] = c;

    return out_matrix;
}

FORCE_INLINE mat4
mat4_euler_z(f32 roll_radians) {
    mat4 out_matrix = mat4_identity();

    f32 c = math_cos(roll_radians);
    f32 s = math_sin(roll_radians);

    out_matrix.elements[0] = c;
    out_matrix.elements[1] = s;
    out_matrix.elements[4] = -s;
    out_matrix.elements[5] = c;

    return out_matrix;
}

FORCE_INLINE mat4
mat4_euler_xyz(f32 yaw_radians, f32 pitch_radians, f32 roll_radians) {

    mat4 rx = mat4_euler_x(yaw_radians);
    mat4 ry = mat4_euler_y(pitch_radians);
    mat4 rz = mat4_euler_z(roll_radians);

    mat4 out_matrix = rx * ry * rz;

    return out_matrix;
}

FORCE_INLINE vec3
mat4_forward(mat4 matrix) {
    vec3 forward;
    forward.x = -matrix.elements[2];
    forward.y = -matrix.elements[6];
    forward.z = -matrix.elements[10];
    vec3_norm(&forward);
    return forward;
}

FORCE_INLINE vec3
mat4_backward(mat4 matrix) {
    vec3 backward;
    backward.x = matrix.elements[2];
    backward.y = matrix.elements[6];
    backward.z = matrix.elements[10];
    vec3_norm(&backward);
    return backward;
}

FORCE_INLINE vec3
mat4_up(mat4 matrix) {
    vec3 up;
    up.x = matrix.elements[1];
    up.y = matrix.elements[5];
    up.z = matrix.elements[9];
    vec3_norm(&up);
    return up;
}

FORCE_INLINE vec3
mat4_down(mat4 matrix) {
    vec3 down;
    down.x = -matrix.elements[1];
    down.y = -matrix.elements[5];
    down.z = -matrix.elements[9];
    vec3_norm(&down);
    return down;
}

FORCE_INLINE vec3
mat4_left(mat4 matrix) {
    vec3 left;
    left.x = -matrix.elements[0];
    left.y = -matrix.elements[4];
    left.z = -matrix.elements[8];
    vec3_norm(&left);
    return left;
}

FORCE_INLINE vec3
mat4_right(mat4 matrix) {
    vec3 right;
    right.x = matrix.elements[0];
    right.y = matrix.elements[4];
    right.z = matrix.elements[8];
    vec3_norm(&right);
    return right;
}

// Quaternion functions
FORCE_INLINE quaternion
quat_identity() {
    quaternion result = {0, 0, 0, 1.0f};
    return result;
}

FORCE_INLINE f32
quat_normal(quaternion q) {
    return math_sqrt(q.qx * q.qx + q.qy * q.qy + q.qz * q.qz + q.qw * q.qw);
}

FORCE_INLINE quaternion
quat_norm(quaternion q) {
    f32 normal = quat_normal(q);
    quaternion result = {q.qx / normal,
        q.qy / normal,
        q.qz / normal,
        q.qw / normal};
    return result;
}

FORCE_INLINE quaternion
quat_conjugate(quaternion q) {
    quaternion result = {-q.qx, -q.qy, -q.qz, q.qw};
    return result;
}

FORCE_INLINE quaternion
quat_inv(quaternion q) {
    return quat_norm(quat_conjugate(q));
}

FORCE_INLINE quaternion
quat_mul(quaternion q0, quaternion q1) {
    quaternion out_quaternion;

    out_quaternion.x =
        q0.qx * q1.qw + q0.qy * q1.qz - q0.qz * q1.qy + q0.qw * q1.qx;

    out_quaternion.y =
        -q0.qx * q1.qz + q0.qy * q1.qw + q0.qz * q1.qx + q0.qw * q1.qy;

    out_quaternion.z =
        q0.qx * q1.qy - q0.qy * q1.qx + q0.qz * q1.qw + q0.qw * q1.qz;

    out_quaternion.w =
        -q0.qx * q1.qx - q0.qy * q1.qy - q0.qz * q1.qz + q0.qw * q1.qw;

    return out_quaternion;
}

FORCE_INLINE f32
quat_dot(quaternion q0, quaternion q1) {

    return q0.qx * q1.qx + q0.qy * q1.qy + q0.qz * q1.qz + q0.qw * q1.qw;
}

FORCE_INLINE mat4
quat_to_mat4(quaternion q) {

    mat4 out_matrix = mat4_identity();

    // https://stackoverflow.com/questions/1556260/convert-quaternion-rotation-to-rotation-matrix

    quaternion n = quat_norm(q);

    out_matrix.elements[0] = 1.0f - 2.0f * n.qy * n.qy - 2.0f * n.qz * n.qz;
    out_matrix.elements[1] = 2.0f * n.qx * n.qy - 2.0f * n.qz * n.qw;
    out_matrix.elements[2] = 2.0f * n.qx * n.qz + 2.0f * n.qy * n.qw;
    out_matrix.elements[4] = 2.0f * n.qx * n.qy + 2.0f * n.qz * n.qw;

    out_matrix.elements[5] = 1.0f - 2.0f * n.qx * n.qx - 2.0f * n.qz * n.qz;
    out_matrix.elements[6] = 2.0f * n.qy * n.qz - 2.0f * n.qx * n.qw;
    out_matrix.elements[8] = 2.0f * n.qx * n.qz - 2.0f * n.qy * n.qw;
    out_matrix.elements[9] = 2.0f * n.qy * n.qz + 2.0f * n.qx * n.qw;

    out_matrix.elements[10] = 1.0f - 2.0f * n.qx * n.qx - 2.0f * n.qy * n.qy;

    return out_matrix;
}

// Calculates a rotation matrix based on the quaternion and the passed in center
// point.
FORCE_INLINE mat4
quat_to_rotation_matrix(quaternion q, vec3 center) {

    mat4 out_matrix;

    f32 *o = out_matrix.elements;

    o[0] = (q.qx * q.qx) - (q.qy * q.qy) - (q.qz * q.qz) + (q.qw * q.qw);
    o[1] = 2.0f * ((q.qx * q.qy) + (q.qz * q.qw));
    o[2] = 2.0f * ((q.qx * q.qz) - (q.qy * q.qw));
    o[3] = center.x - center.x * o[0] - center.y * o[1] - center.z * o[2];

    o[4] = 2.0f * ((q.qx * q.qy) - (q.qz * q.qw));
    o[5] = -(q.qx * q.qx) + (q.qy * q.qy) - (q.qz * q.qz) + (q.qw * q.qw);
    o[6] = 2.0f * ((q.qy * q.qz) + (q.qx * q.qw));
    o[7] = center.y - center.x * o[4] - center.y * o[5] - center.z * o[6];

    o[8] = 2.0f * ((q.qx * q.qz) + (q.qy * q.qw));
    o[9] = 2.0f * ((q.qy * q.qz) - (q.qx * q.qw));
    o[10] = -(q.qx * q.qx) - (q.qy * q.qy) + (q.qz * q.qz) + (q.qw * q.qw);
    o[11] = center.z - center.x * o[8] - center.y * o[9] - center.z * o[10];

    o[12] = 0.0f;
    o[13] = 0.0f;
    o[14] = 0.0f;
    o[15] = 1.0f;

    return out_matrix;
}

FORCE_INLINE quaternion
quat_from_axis_angle(vec3 axis, f32 angle, b8 normalize) {

    const f32 half_angle = 0.5f * angle;

    f32 s = math_sin(half_angle);

    f32 c = math_cos(half_angle);

    quaternion q = {s * axis.x, s * axis.y, s * axis.z, c};

    if (normalize) {
        return quat_norm(q);
    }

    return q;
}

FORCE_INLINE quaternion
quat_slerp(quaternion q0, quaternion q1, f32 percentage) {

    quaternion out_quaternion;

    // Source: https://en.wikipedia.org/wiki/Slerp
    // Only unit quaternions are valid rotations. Normalize to avoid undefined
    // behavior.
    quaternion v0 = quat_norm(q0);

    quaternion v1 = quat_norm(q1);

    // Compute the cosine of the angle between the two vectors.
    f32 dot = quat_dot(v0, v1);

    // If the dot product is negative, slerp won't take the shorter path. Note
    // that v1 and -v1 are equivalent when the negation is applied to all four
    // components. Fix by reversing one quaternion.

    if (dot < 0.0f) {
        v1.x = -v1.x;
        v1.y = -v1.y;
        v1.z = -v1.z;
        v1.w = -v1.w;
        dot = -dot;
    }

    const f32 DOT_THRESHOLD = 0.9995f;

    if (dot > DOT_THRESHOLD) {

        // If the inputs are too close for comfort, linearly interpolate
        // and normalize the result.
        out_quaternion = {v0.x + ((v1.x - v0.x) * percentage),
            v0.y + ((v1.y - v0.y) * percentage),
            v0.z + ((v1.z - v0.z) * percentage),
            v0.w + ((v1.w - v0.w) * percentage)};

        return quat_norm(out_quaternion);
    }

    // Since dot is in range [0, DOT_THRESHOLD], acos is safe
    f32 theta_0 = math_arccos(dot); // theta_0 = angle between input vectors

    f32 theta = theta_0 * percentage; // theta = angle between v0 and result

    f32 sin_theta = math_sin(theta); // compute this value only once

    f32 sin_theta_0 = math_sin(theta_0); // compute this value only once

    f32 s0 =
        math_cos(theta) -
        dot * sin_theta / sin_theta_0; // == sin(theta_0 - theta) / sin(theta_0)

    f32 s1 = sin_theta / sin_theta_0;

    quaternion result = {(v0.x * s0) + (v1.x * s1),
        (v0.y * s0) + (v1.y * s1),
        (v0.z * s0) + (v1.z * s1),
        (v0.w * s0) + (v1.w * s1)};

    return result;
}

FORCE_INLINE f32
deg_to_rad(f32 degrees) {
    return degrees * math::DEG_RAD_FACTOR;
}

FORCE_INLINE f32
rad_to_deg(f32 radians) {
    return radians * math::RAD_DEG_FACTOR;
}
