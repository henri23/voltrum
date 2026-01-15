#include "image_loader.hpp"
#include "core/asserts.hpp"
#include "core/logger.hpp"
#include "memory/memory.hpp"
#include "resources/resource_types.hpp"
#include "systems/resource_system.hpp"
#include "utils/string.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

INTERNAL_FUNC b8 image_loader_load(struct Resource_Loader *self,
    const char *name,
    Resource *out_resource) {

    if (!self || !name || !out_resource) {
        CORE_ERROR("image_loader_load - Ensure all pointers are not nullptr");
        return false;
    }

    const char *format_str = "%s/%s/%s%s";
    const s32 required_channel_count = 4; // RGBA

    // NOTE: Most images are stored in a format where the data is actually
    // stored updside-down, so if we load the data from the bottom -> up we
    // should technically retrieve the original orientation
    stbi_set_flip_vertically_on_load(true);
    char full_file_path[512];

    // TODO: Loop over different file extensions
    string_format(full_file_path,
        format_str,
        resource_system_base_path(),
        self->type_path,
        name,
        ".png");

    s32 width;
    s32 height;
    s32 channel_count;

    // WARN: This load operation is assuming 8 bits per channel and 8 channels.
    // Moreover stbi_load will use malloc in the backend to allocate the pixels
    // in the heap. Basically we can just use this array and use memory_dealloc
    // since we are using malloc too in the backend, but to make the memory
    // alloc more explicit I am creating a new buffer and copying over the data
    // from stbi and using stbi_image_free to free the data so that we
    // completelly shift to the voltrum memory management system and do not have
    // inconsitencies when I potentially change the memory management system
    u8 *data = stbi_load(full_file_path,
        &width,
        &height,
        &channel_count,
        required_channel_count);

    const char *fail_reason = stbi_failure_reason();
    if (fail_reason) {
        CORE_ERROR("Image resource loader failed to load file '%s': '%s'",
            full_file_path,
            fail_reason);

        stbi__err(0, 0);

        if (data) {
            stbi_image_free(data);
        }

        return false;
    }

    if (!data) {
        CORE_ERROR("Image resource loader failed to load file '%s'",
            full_file_path);

        return false;
    }

    // Calculate pixel data size
    u32 pixel_data_size = width * height * required_channel_count;

    // Copy stbi data to our memory system
    u8 *pixels = static_cast<u8 *>(
        memory_allocate(pixel_data_size, Memory_Tag::TEXTURE));
    memory_copy(pixels, data, pixel_data_size);

    // Free stbi allocated memory immediately
    stbi_image_free(data);

    // TODO : Its better to switch to an allocator for this
    out_resource->full_path = string_duplicate(full_file_path);

    // TODO: Allocator
    Image_Resource_Data *resource_data = static_cast<Image_Resource_Data *>(
        memory_allocate(sizeof(Image_Resource_Data), Memory_Tag::TEXTURE));

    resource_data->pixels = pixels;
    resource_data->width = width;
    resource_data->height = height;
    resource_data->channel_count = required_channel_count;

    out_resource->data = resource_data;
    out_resource->data_size = sizeof(Image_Resource_Data);
    out_resource->name = name;

    return true;
}

INTERNAL_FUNC void image_loader_unload(struct Resource_Loader *self,
    Resource *resource) {
    if (!self || !resource) {
        CORE_WARN(
            "image_loader_unload called with nullptr for self of resource.");
        return;
    }

    u32 path_length = string_length(resource->full_path);
    if (path_length) {
        memory_deallocate(resource->full_path,
            sizeof(char) * path_length,
            Memory_Tag::STRING);
    }

    if (resource->data) {
        Image_Resource_Data *image_data =
            static_cast<Image_Resource_Data *>(resource->data);

        // Free the pixel data first
        if (image_data->pixels) {
            u32 pixel_data_size = image_data->width * image_data->height *
                                  image_data->channel_count;
            memory_deallocate(image_data->pixels,
                pixel_data_size,
                Memory_Tag::TEXTURE);
            image_data->pixels = nullptr;
        }

        // Free the resource data structure
        memory_deallocate(resource->data,
            resource->data_size,
            Memory_Tag::TEXTURE);

        resource->data = nullptr;
        resource->data_size = 0;
        resource->loader_id = INVALID_ID;
    }
}

Resource_Loader image_resource_loader_create() {
    Resource_Loader loader;
    loader.type = Resource_Type::IMAGE;
    loader.load = image_loader_load;
    loader.unload = image_loader_unload;
    loader.type_path = "textures";

    return loader;
}
