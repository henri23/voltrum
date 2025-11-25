#pragma once

#include "defines.hpp"

struct vec2 {
    union {
        struct {
            f32 x, y;
        };
        struct {
            f32 u, v;
        };
        struct {
            f32 s, t;
        };
        f32 elements[2];
    };
};

struct vec3 {
    union {
        struct {
            f32 x, y, z;
        };
        struct {
            f32 r, g, b;
        };
        struct {
            f32 s, t, p;
        };
        struct {
            f32 u, v, w;
        };
        f32 elements[3];
    };
};

// In 3D space we cannot distinguish just from the data itself, whether a vec3
// represents a coordinate or a direction. Sometimes homogeneous vectors are
// used which have a 4th dimension that is 1 for positions, and for directions
// the 4th dimension will be 0. Needless to say, this 4th dimension will be
// subject to linear transformations too, just like the others coordinates

// Another use case of vec4 is for representing 3D rotations with quaternions
// rather than Euler angles
struct vec4 {
    union {
        struct {
            f32 x, y, z, w;
        };
        struct {
            f32 r, g, b, a;
        };
        struct {
            f32 s, t, p, q;
        };
        struct {
            f32 qx, qy, qz, qw;
        }; // Quaternion representation
        f32 elements[4];
    };
};

// typedef vec4 quaternion;
using quaternion = vec4;

struct mat4 {
    alignas(16) f32 elements[16];
};

struct vertex_3d {
    vec3 position;
    vec2 texture_coordinate;
};
