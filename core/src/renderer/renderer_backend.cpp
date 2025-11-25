#include "renderer/renderer_backend.hpp"

#include "vulkan/vulkan_backend.hpp"

b8 renderer_backend_initialize(Renderer_Backend_Type type,
    Renderer_Backend* out_backend) {

    // Instantiate specific backend
    switch (type) {
    case Renderer_Backend_Type::VULKAN: {
        // Renderer logic
        out_backend->initialize = vulkan_initialize;
        out_backend->shutdown = vulkan_shutdown;
        out_backend->resized = vulkan_on_resized;
        out_backend->begin_frame = vulkan_begin_frame;
        out_backend->end_frame = vulkan_end_frame;

        // Old texture methods
        out_backend->create_ui_image = vulkan_create_ui_image;
        out_backend->destroy_ui_image = vulkan_destroy_ui_image;

        // Descriptors
        out_backend->update_global_state = vulkan_update_global_state;
        out_backend->draw_geometry = vulkan_draw_geometry;

        out_backend->create_texture = vulkan_create_texture;
        out_backend->destroy_texture = vulkan_destroy_texture;

        out_backend->create_material = vulkan_create_material;
        out_backend->destroy_material = vulkan_destroy_material;

        out_backend->create_geometry = vulkan_create_geometry;
        out_backend->destroy_geometry = vulkan_destroy_geometry;

        return true;
    }
    case Renderer_Backend_Type::OPENGL:
    case Renderer_Backend_Type::DIRECTX:
        break;
    }

    return false;
}

void renderer_backend_shutdown(Renderer_Backend* backend) {
    backend->initialize = nullptr;
    backend->shutdown = nullptr;
    backend->resized = nullptr;
    backend->begin_frame = nullptr;
    backend->end_frame = nullptr;
    backend->update_global_state = nullptr;

    // TODO: Delete
    backend->create_ui_image = nullptr;
    backend->destroy_ui_image = nullptr;

    backend->draw_geometry = nullptr;

    backend->create_texture = nullptr;
    backend->destroy_texture = nullptr;

    backend->create_material = nullptr;
    backend->destroy_material = nullptr;

    backend->create_geometry = nullptr;
    backend->destroy_geometry = nullptr;
}
