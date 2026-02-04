#include "renderer/renderer_frontend.hpp"

#include "core/frame_context.hpp"
#include "core/logger.hpp"
#include "defines.hpp"
#include "math/math.hpp"
#include "math/math_types.hpp"
#include "renderer/renderer_backend.hpp"
#include "renderer/renderer_types.hpp"
#include "renderer/vulkan/vulkan_types.hpp"

struct Renderer_System_State
{
    Renderer_Backend backend;
    mat4             projection;

    // Cached value of the camera transformation managed in the client
    mat4 view;

    f32 near_clip;
    f32 far_clip;
};

internal_var Renderer_System_State *state_ptr;

Renderer_System_State *
renderer_init(
    Arena          *allocator,
    Platform_State *platform,
    String          application_name)
{
    auto *state = push_struct(allocator, Renderer_System_State);

    state->near_clip = 0.1f;
    state->far_clip  = 1000.0f;

    if (!renderer_backend_initialize(Renderer_Backend_Type::VULKAN,
                                     allocator,
                                     &state->backend))
    {
        CORE_INFO("Failed to initialize renderer backend");
        return nullptr;
    }

    state->backend.initialize(allocator, platform, application_name);

    state->projection = mat4_project_perspective(deg_to_rad(45.0f),
                                                 1280 / 720.0f,
                                                 state->near_clip,
                                                 state->far_clip);

    // Default view to identity until the client provides a camera
    state->view = mat4_identity();

    state_ptr = state;

    CORE_DEBUG("Renderer subsystem initialized");
    return state;
}

// Window resize - only notify backend about swapchain resize
// Projection matrix is updated by renderer_resize_viewport() based on
// viewport dimensions
void
renderer_on_resize(u16 width, u16 height)
{
    state_ptr->backend.resized(width, height);
}

b8
renderer_draw_frame(Frame_Context *frame_ctx, Render_Context *render_ctx)
{
    if (state_ptr->backend.begin_frame(frame_ctx, frame_ctx->delta_t))
    {

        if (!state_ptr->backend.start_renderpass(frame_ctx,
                                                 Renderpass_Type::VIEWPORT))
        {
            CORE_ERROR(
                "backend.start_renderpass - Viewport renderpass failed. "
                "Application shutting down...");
            return false;
        }

        state_ptr->backend.update_global_viewport_state(state_ptr->projection,
                                                        state_ptr->view,
                                                        vec3_zero(),
                                                        vec4_one(),
                                                        0);

        // Draw geometries
        u32 count = render_ctx->geometry_count;
        for (u32 i = 0; i < count; ++i)
        {
            state_ptr->backend.draw_geometry(render_ctx->geometries[i]);
        }

        if (!state_ptr->backend.finish_renderpass(frame_ctx,
                                                  Renderpass_Type::VIEWPORT))
        {
            CORE_ERROR(
                "backend.end_renderpass - Viewport renderpass failed. "
                "Appplication shutting down...");
        }

        if (!state_ptr->backend.start_renderpass(frame_ctx,
                                                 Renderpass_Type::UI))
        {
            CORE_ERROR(
                "backend.start_renderpass - UI renderpass failed. "
                "Application shutting down...");
            return false;
        }

        // Draw UI geometry
        if (render_ctx->ui_data.draw_list)
        {
            state_ptr->backend.draw_ui(render_ctx->ui_data);
        }

        if (!state_ptr->backend.finish_renderpass(frame_ctx,
                                                  Renderpass_Type::UI))
        {
            CORE_ERROR(
                "backend.start_renderpass - UI renderpass failed. "
                "Application shutting down...");
            return false;
        }

        b8 result = state_ptr->backend.end_frame(frame_ctx, frame_ctx->delta_t);
        state_ptr->backend.frame_number++;

        if (!result)
        {
            CORE_ERROR(
                "renderer_end_frame failed. Application shutting down...");
            return false;
        }
    }

    return true;
}

void
renderer_set_view(mat4 view)
{
    state_ptr->view = view;
}

void
renderer_create_texture(const u8       *pixels,
                        struct Texture *texture,
                        b8              is_ui_texture)
{
    state_ptr->backend.create_texture(pixels, texture, is_ui_texture);
}

void
renderer_destroy_texture(struct Texture *texture)
{
    state_ptr->backend.destroy_texture(texture);
}

void *
renderer_get_texture_draw_data(struct Texture *texture)
{
    if (!texture || !texture->internal_data)
    {
        return nullptr;
    }

    Vulkan_Texture_Data *data = (Vulkan_Texture_Data *)texture->internal_data;
    return (void *)(intptr_t)data->ui_descriptor_set;
}

b8
renderer_create_material(struct Material *material)
{
    return state_ptr->backend.create_material(material);
}

void
renderer_destroy_material(struct Material *material)
{
    return state_ptr->backend.destroy_material(material);
}

b8
renderer_create_geometry(Geometry        *geometry,
                         u32              vertex_count,
                         const vertex_3d *vertices,
                         u32              index_count,
                         u32             *indices)
{
    return state_ptr->backend.create_geometry(geometry,
                                              vertex_count,
                                              vertices,
                                              index_count,
                                              indices);
}

void
renderer_destroy_geometry(Geometry *geometry)
{
    state_ptr->backend.destroy_geometry(geometry);
}

void
renderer_render_viewport()
{
    state_ptr->backend.render_viewport();
}

void *
renderer_get_rendered_viewport()
{
    return state_ptr->backend.get_rendered_viewport();
}

void
renderer_resize_viewport(u32 width, u32 height)
{
    state_ptr->backend.resize_viewport(width, height);

    // Update projection matrix for new aspect ratio
    if (height > 0)
    {
        f32 aspect            = width / (f32)height;
        state_ptr->projection = mat4_project_perspective(deg_to_rad(45.0f),
                                                         aspect,
                                                         state_ptr->near_clip,
                                                         state_ptr->far_clip);
    }
}

void
renderer_get_viewport_size(u32 *width, u32 *height)
{
    state_ptr->backend.get_viewport_size(width, height);
}
