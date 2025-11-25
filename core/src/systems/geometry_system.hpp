#pragma once

#include "defines.hpp"
#include "resources/resource_types.hpp"

constexpr const char* DEFAULT_GEOMETRY_NAME = "default_";

struct Geometry_System_Config {
    // NOTE: The geometry count should be significantly higher than the static
    // meshes becaus there can and will be more than one of these per mesh. This
    // is because some meshes can be made of many subobject and can contain even
    // hundreds of static meshes
    u32 max_geometry_count;
};

// This is the geometry config inteself for the geometry to be drawn
struct Geometry_Config {
    u32 vertex_count;
    vertex_3d* vertices;
    u32 index_count;
    u32* indices;
    char name[GEOMETRY_NAME_MAX_LENGTH];
    char material_name[MATERIAL_NAME_MAX_LENGTH];
};

b8 geometry_system_init(Geometry_System_Config config);
void geometry_system_shutdown();

Geometry* geometry_system_acquire_by_id(Geometry_ID id);

Geometry* geometry_system_acquire_by_config(Geometry_Config config,
    b8 auto_release);

void geometry_release(Geometry* geometry);

Geometry* geometry_system_get_default();

// Creates configuration for plane geometries given the provided parameters.
// WARN: The vertex and index arrays are dynamically allocated and should be
// freed upon object disposal
Geometry_Config geometry_system_generate_plane_config(f32 width,
    f32 height,
    u32 x_segment_count,
    u32 y_segment_count,
    f32 tile_x,
    f32 tile_y,
    const char* name,
    const char* material_name);
