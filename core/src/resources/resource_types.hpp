#pragma once

#include "math/math_types.hpp"
#include "utils/enum.hpp"

using Texture_ID = u32;
constexpr u32 TEXTURE_NAME_MAX_LENGTH = 256;

// Interface
struct Texture {
    Texture_ID id;
    u32 width;
    u32 height;
    u8 channel_count;
    b8 has_transparency;
    u32 generation;
    char name[TEXTURE_NAME_MAX_LENGTH];
    void* internal_data; // Graphics API specific data
};

// Bitmask
enum class Texture_Type {
    UNKNOWN = 0x00,
    MAP_DIFFUSE = 1 << 0,
    // MAP_DIFFUSE = 1 << 1,
};

ENABLE_BITMASK(Texture_Type); // Enable c++ enum to be used as bitmask

struct Texture_Map {
    Texture* texture;
    Texture_Type type;
};

using Material_ID = u32;
constexpr u32 MATERIAL_NAME_MAX_LENGTH = 256;

struct Material {
    Material_ID id;
    u32 generation;
    u32 internal_id; // Renderer specific object identifier
    char name[MATERIAL_NAME_MAX_LENGTH];
    vec4 diffuse_color;
    Texture_Map diffuse_map;
};

using Geometry_ID = u32;
constexpr u32 GEOMETRY_NAME_MAX_LENGTH = 256;

struct Geometry {
    Geometry_ID id;
    u32 internal_id;
    u32 generation;
    char name[GEOMETRY_NAME_MAX_LENGTH];
    Material* material;
};
