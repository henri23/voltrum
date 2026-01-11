#pragma once

#include "math/math_types.hpp"
#include "utils/enum.hpp"

enum class Resource_Type : u32 {
    TEXT,        // the data will be a char* array
    BINARY,      // the data will be u8* array
    IMAGE,       // the data will be Image_Resource_Data
    ICON,        // the data will be Image_Resource_Data (no vertical flip)
    MATERIAL,    // the data  will be Material_Config
    STATIC_MESH, // the data will be TBD
    FONT,        // the data will be u8* array
    CUSTOM
};

using Loader_ID = u32;

// The resource loader will be responsible for temporaril allocating the data
// of the resource and deleting it
struct Resource {
    Loader_ID loader_id;
    const char* name;
    char* full_path;
    u64 data_size; // Size of the data stored in the opaque member void* data
    void* data;
};

struct Image_Resource_Data {
    u8 channel_count;
    u32 width;
    u32 height;
    u8* pixels;
};

using Texture_ID = u32;
constexpr u32 TEXTURE_NAME_MAX_LENGTH = 256;

struct Texture {
    Texture_ID id;
    u32 width;
    u32 height;
    u8 channel_count;
    b8 has_transparency;
    b8 is_ui_texture;
    u32 generation;
    char name[TEXTURE_NAME_MAX_LENGTH];
    void* internal_data;
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

struct Material_Config {
    char name[MATERIAL_NAME_MAX_LENGTH];
    b8 auto_release;
    vec4 diffuse_color;
    char diffuse_map_name[TEXTURE_NAME_MAX_LENGTH];
};

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
