#include "renderer/renderer_backend.hpp"

#include "vulkan/vulkan_backend.hpp"

b8
renderer_backend_initialize(Renderer_Backend_Type type,
                            Arena                *allocator,
                            Renderer_Backend     *out_backend)
{
    // Instantiate specific backend
    switch (type)
    {
    case Renderer_Backend_Type::VULKAN:
    {
        // Renderer logic
        out_backend->initialize  = vulkan_initialize;
        out_backend->shutdown    = vulkan_shutdown;
        out_backend->resized     = vulkan_on_resized;
        out_backend->begin_frame = vulkan_begin_frame;
        out_backend->end_frame   = vulkan_end_frame;

        // Descriptors
        out_backend->update_global_viewport_state =
            vulkan_update_global_viewport_state;
        out_backend->draw_geometry            = vulkan_draw_geometry;
        out_backend->draw_grid                = vulkan_draw_grid;
        out_backend->draw_ui                  = vulkan_draw_ui;
        out_backend->set_viewport_clear_color = vulkan_set_viewport_clear_color;

        out_backend->start_renderpass  = vulkan_renderpass_start;
        out_backend->finish_renderpass = vulkan_renderpass_finish;

        out_backend->create_texture  = vulkan_create_texture;
        out_backend->destroy_texture = vulkan_destroy_texture;

        out_backend->create_material  = vulkan_create_material;
        out_backend->destroy_material = vulkan_destroy_material;

        out_backend->create_geometry  = vulkan_create_geometry;
        out_backend->destroy_geometry = vulkan_destroy_geometry;

        // Viewport management
        out_backend->render_viewport       = vulkan_render_viewport;
        out_backend->get_rendered_viewport = vulkan_get_rendered_viewport;
        out_backend->resize_viewport       = vulkan_resize_viewport;
        out_backend->get_viewport_size     = vulkan_get_viewport_size;

        return true;
    }
    case Renderer_Backend_Type::OPENGL:
    case Renderer_Backend_Type::DIRECTX:
        break;
    }

    return false;
}
