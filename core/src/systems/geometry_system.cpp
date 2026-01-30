#include "geometry_system.hpp"

#include "core/asserts.hpp"
#include "core/logger.hpp"
#include "memory/memory.hpp"
#include "renderer/renderer_frontend.hpp"
#include "systems/material_system.hpp"
#include "utils/string.hpp"

internal_var Geometry_System_State *state_ptr;

INTERNAL_FUNC b8 create_default_geometry(Geometry_System_State *state);
INTERNAL_FUNC b8 create_geometry(Geometry_System_State *state,
                                 Geometry_Config        config,
                                 Geometry              *geometry);

INTERNAL_FUNC void destroy_geometry(Geometry_System_State *state,
                                    Geometry              *geometry);

Geometry_System_State *
geometry_system_init(Arena *allocator, Geometry_System_Config config)
{
    u32 count = config.max_geometry_count;

    RUNTIME_ASSERT_MSG(
        count > 0,
        "geometry_system_init - config.max_geometry_count must be > 0");

    auto *state = push_struct(allocator, Geometry_System_State);

    state->config = config;

    state->registered_geometries =
        push_array(allocator, Geometry_Reference, count);

    // Invalidate all ids present in the geometry array
    for (u32 i = 0; i < count; ++i)
    {
        state->registered_geometries[i].geometry.id          = INVALID_ID;
        state->registered_geometries[i].geometry.generation  = INVALID_ID;
        state->registered_geometries[i].geometry.internal_id = INVALID_ID;
    }

    if (!create_default_geometry(state))
    {
        CORE_FATAL("Failed to create default geometry. Application must abort");
        return nullptr;
    }

    state_ptr = state;

    return state;
}

Geometry *
geometry_system_acquire_by_id(Geometry_ID id)
{
    // NOTE: No need to write the branch for the geometry not being present
    // because since we are querying by id, it means as some previous point we
    // must have acquired the geometry from config and received an id for it
    if (id != INVALID_ID &&
        state_ptr->registered_geometries[id].geometry.id != INVALID_ID)
    {
        state_ptr->registered_geometries[id].reference_count++;
        return &state_ptr->registered_geometries[id].geometry;
    }

    CORE_ERROR(
        "geometry_system_acquire_by_id cannot load invalid geometry id. "
        "Returning nullptr");
    return nullptr;
}

Geometry *
geometry_system_acquire_by_config(Geometry_Config config, b8 auto_release)
{

    Geometry *geometry = nullptr;

    for (u32 i = 0; i < state_ptr->config.max_geometry_count; ++i)
    {
        if (state_ptr->registered_geometries[i].geometry.id == INVALID_ID)
        {
            // Empty slot
            state_ptr->registered_geometries[i].auto_release    = auto_release;
            state_ptr->registered_geometries[i].reference_count = 1;
            geometry     = &state_ptr->registered_geometries[i].geometry;
            geometry->id = i;
            break;
        }
    }

    if (!geometry)
    {
        CORE_ERROR(
            "Geometry registry is full. Adjust config to allow more registered "
            "geometries");
        return nullptr;
    }

    if (!create_geometry(state_ptr, config, geometry))
    {
        CORE_ERROR("Failed to create geometry. Returning nullptr");
        return nullptr;
    }

    return geometry;
}

void
geometry_release(Geometry *geometry)
{
    if (geometry->id != INVALID_ID)
    {
        u32 id = geometry->id;

        Geometry_Reference *ref = &state_ptr->registered_geometries[id];
        if (ref->geometry.id == id)
        {
            if (ref->reference_count > 0)
            {
                ref->reference_count--;
            }
        }

        if (ref->reference_count < 1 && ref->auto_release)
        {
            destroy_geometry(state_ptr, &ref->geometry);
            ref->reference_count = 0;
            ref->auto_release    = false;
        }
        else
        {
            CORE_FATAL("Geometry id doesn't match. Check registration logic");
        }

        return;
    }

    CORE_WARN(
        "geometry_release cannot load release invalid geometry. Skipping.");
}

Geometry *
geometry_system_get_default()
{
    return &state_ptr->default_geometry;
}

// Creates configuration for plane geometries given the provided parameters.
// WARN: The vertex and index arrays are dynamically allocated and should be
// freed upon object disposal
Geometry_Config
geometry_system_generate_plane_config(f32         width,
                                      f32         height,
                                      u32         x_segment_count,
                                      u32         y_segment_count,
                                      f32         tile_x,
                                      f32         tile_y,
                                      const char *name,
                                      const char *material_name)
{
    if (width == 0)
    {
        CORE_WARN("Width must be > 0. Defaulting to one");
        width = 1.0f;
    }

    if (height == 0)
    {
        CORE_WARN("Height must be > 0. Defaulting to one");
        height = 1.0f;
    }

    if (x_segment_count < 1)
    {
        CORE_WARN(
            "x_segment_count must be a positive number. Defaulting to one");
        x_segment_count = 1;
    }

    if (y_segment_count < 1)
    {
        CORE_WARN(
            "y_segment_count must be a positive number. Defaulting to one");
        y_segment_count = 1;
    }

    if (tile_x == 0)
    {
        CORE_WARN("tile_x must be > 0. Defaulting to one.");
        tile_x = 1.0f;
    }

    if (tile_y == 0)
    {
        CORE_WARN("tile_y must be > 0. Defaulting to one.");
        tile_y = 1.0f;
    }

    Geometry_Config config;
    // 4 verts per quad segment
    config.vertex_count = x_segment_count * y_segment_count * 4;
    config.vertices     = static_cast<vertex_3d *>(
        memory_allocate(sizeof(vertex_3d) * config.vertex_count,
                        Memory_Tag::ARRAY));
    // 6 indices per segment
    config.index_count = x_segment_count * y_segment_count * 6;
    config.indices     = static_cast<u32 *>(
        memory_allocate(sizeof(u32) * config.index_count, Memory_Tag::ARRAY));

    f32 seg_width   = width / x_segment_count;
    f32 seg_height  = height / y_segment_count;
    f32 half_width  = width * 0.5f;
    f32 half_height = height * 0.5f;

    for (u32 y = 0; y < y_segment_count; ++y)
    {
        for (u32 x = 0; x < x_segment_count; ++x)
        {
            f32 min_x = (x * seg_width) - half_width;   // Left
            f32 min_y = (y * seg_height) - half_height; // Top

            f32 max_x = min_x + seg_width;  // Right
            f32 max_y = min_y + seg_height; // Bottom

            f32 min_uvx = (x / (f32)x_segment_count) * tile_x;
            f32 min_uvy = (y / (f32)y_segment_count) * tile_y;

            f32 max_uvx = ((x + 1) / (f32)x_segment_count) * tile_x;
            f32 max_uvy = ((y + 1) / (f32)y_segment_count) * tile_y;

            u32        v_offset = ((y * x_segment_count) + x) * 4;
            vertex_3d *v0       = &config.vertices[v_offset + 0];
            vertex_3d *v1       = &config.vertices[v_offset + 1];
            vertex_3d *v2       = &config.vertices[v_offset + 2];
            vertex_3d *v3       = &config.vertices[v_offset + 3];

            v0->position.x            = min_x;
            v0->position.y            = min_y;
            v0->texture_coordinates.x = min_uvx;
            v0->texture_coordinates.y = min_uvy;

            v1->position.x            = max_x;
            v1->position.y            = max_y;
            v1->texture_coordinates.x = max_uvx;
            v1->texture_coordinates.y = max_uvy;

            v2->position.x            = min_x;
            v2->position.y            = max_y;
            v2->texture_coordinates.x = min_uvx;
            v2->texture_coordinates.y = max_uvy;

            v3->position.x            = max_x;
            v3->position.y            = min_y;
            v3->texture_coordinates.x = max_uvx;
            v3->texture_coordinates.y = min_uvy;

            u32 i_offset                 = ((y * x_segment_count) + x) * 6;
            config.indices[i_offset + 0] = v_offset + 0;
            config.indices[i_offset + 1] = v_offset + 1;
            config.indices[i_offset + 2] = v_offset + 2;
            config.indices[i_offset + 3] = v_offset + 0;
            config.indices[i_offset + 4] = v_offset + 3;
            config.indices[i_offset + 5] = v_offset + 1;
        }
    }

    if (name && string_length(name) > 0)
    {
        string_ncopy(config.name, name, GEOMETRY_NAME_MAX_LENGTH);
    }
    else
    {
        string_ncopy(config.name,
                     DEFAULT_GEOMETRY_NAME,
                     GEOMETRY_NAME_MAX_LENGTH);
    }

    if (material_name && string_length(material_name) > 0)
    {
        string_ncopy(config.material_name,
                     material_name,
                     MATERIAL_NAME_MAX_LENGTH);
    }
    else
    {
        string_ncopy(config.material_name,
                     DEFAULT_MATERIAL_NAME,
                     MATERIAL_NAME_MAX_LENGTH);
    }

    return config;
}

INTERNAL_FUNC b8
create_geometry(Geometry_System_State *state,
                Geometry_Config        config,
                Geometry              *geometry)
{
    if (!renderer_create_geometry(geometry,
                                  config.vertex_count,
                                  config.vertices,
                                  config.index_count,
                                  config.indices))
    {

        // Invalidate geometry and reference if the creation on the renderer
        // failed
        state->registered_geometries[geometry->id].reference_count = 0;
        state->registered_geometries[geometry->id].auto_release    = false;
        geometry->id                                               = INVALID_ID;
        geometry->generation                                       = INVALID_ID;
        geometry->internal_id                                      = INVALID_ID;

        return false;
    }

    if (string_length(config.material_name) > 0)
    {
        geometry->material = material_system_acquire(config.material_name);
        if (!geometry->material)
        {
            // If the requested material fails to be acquired, fallback to
            // default material
            geometry->material = material_system_get_default();
        }
    }

    return true;
}

INTERNAL_FUNC void
destroy_geometry(Geometry_System_State *state, Geometry *geometry)
{
    renderer_destroy_geometry(geometry);
    geometry->id          = INVALID_ID;
    geometry->internal_id = INVALID_ID;
    geometry->generation  = INVALID_ID;

    string_empty(geometry->name);

    // Release the material
    if (geometry->material && string_length(geometry->material->name) > 0)
    {
        material_system_release(geometry->material->name);
        geometry->material = nullptr;
    }
}

INTERNAL_FUNC b8
create_default_geometry(Geometry_System_State *state)
{
    vertex_3d verts[4];
    memory_zero(verts, sizeof(vertex_3d) * 4);

    constexpr f32 f = 10.0f;

    verts[0].position.x            = -0.5 * f;
    verts[0].position.y            = -0.5 * f;
    verts[0].texture_coordinates.x = 0.0f;
    verts[0].texture_coordinates.y = 0.0f;

    verts[1].position.x            = 0.5 * f;
    verts[1].position.y            = 0.5 * f;
    verts[1].texture_coordinates.x = 1.0f;
    verts[1].texture_coordinates.y = 1.0f;

    verts[2].position.x            = -0.5 * f;
    verts[2].position.y            = 0.5 * f;
    verts[2].texture_coordinates.x = 0.0f;
    verts[2].texture_coordinates.y = 1.0f;

    verts[3].position.x            = 0.5 * f;
    verts[3].position.y            = -0.5 * f;
    verts[3].texture_coordinates.x = 1.0f;
    verts[3].texture_coordinates.y = 0.0f;

    u32 indices[6] = {0, 1, 2, 0, 3, 1};

    if (!renderer_create_geometry(&state->default_geometry,
                                  4,
                                  verts,
                                  6,
                                  indices))
    {
        CORE_FATAL("Failed to create default geometry. Application must abort");
        return false;
    }

    state->default_geometry.material = material_system_get_default();

    return true;
}
