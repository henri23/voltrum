#include "icon_loader.hpp"
#include "core/logger.hpp"
#include "memory/memory.hpp"
#include "resources/resource_types.hpp"
#include "systems/resource_system.hpp"
#include "utils/string.hpp"

#include "stb_image.h"

INTERNAL_FUNC b8 icon_loader_load(struct Resource_Loader* self,
    const char* name,
    Resource* out_resource) {

    if (!self || !name || !out_resource) {
        CORE_ERROR("icon_loader_load - Ensure all pointers are not nullptr");
        return false;
    }

    const char* format_str = "%s/%s/%s%s";
    const s32 required_channel_count = 4; // RGBA

    // Icons should NOT be flipped vertically (unlike textures)
    stbi_set_flip_vertically_on_load(false);

    char full_file_path[512];

    string_format(full_file_path,
        format_str,
        resource_system_base_path(),
        self->type_path,
        name,
        ".png");

    s32 width;
    s32 height;
    s32 channel_count;

    u8* data = stbi_load(full_file_path,
        &width,
        &height,
        &channel_count,
        required_channel_count);

    if (!data) {
        const char* fail_reason = stbi_failure_reason();
        CORE_ERROR("Icon resource loader failed to load file '%s': '%s'",
            full_file_path,
            fail_reason ? fail_reason : "unknown error");

        return false;
    }

    // Calculate pixel data size
    u32 pixel_data_size = width * height * required_channel_count;

    // Copy stbi data to our memory system
    u8* pixels =
        static_cast<u8*>(memory_allocate(pixel_data_size, Memory_Tag::TEXTURE));
    memory_copy(pixels, data, pixel_data_size);

    // Free stbi allocated memory immediately
    stbi_image_free(data);

    out_resource->full_path = string_duplicate(full_file_path);

    Image_Resource_Data* resource_data = static_cast<Image_Resource_Data*>(
        memory_allocate(sizeof(Image_Resource_Data), Memory_Tag::TEXTURE));

    resource_data->pixels = pixels;
    resource_data->width = width;
    resource_data->height = height;
    resource_data->channel_count = required_channel_count;

    out_resource->data = resource_data;
    out_resource->data_size = sizeof(Image_Resource_Data);
    out_resource->name = name;

    CORE_DEBUG("Icon loaded: %s (%dx%d)", name, width, height);

    return true;
}

INTERNAL_FUNC void icon_loader_unload(struct Resource_Loader* self,
    Resource* resource) {
    if (!self || !resource) {
        CORE_WARN(
            "icon_loader_unload called with nullptr for self or resource.");
        return;
    }

    u64 path_length = string_length(resource->full_path);
    if (path_length) {
        memory_deallocate(resource->full_path,
            sizeof(char) * path_length,
            Memory_Tag::STRING);
    }

    if (resource->data) {
        Image_Resource_Data* image_data =
            static_cast<Image_Resource_Data*>(resource->data);

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

Resource_Loader icon_resource_loader_create() {
    Resource_Loader loader;
    loader.type = Resource_Type::ICON;
    loader.load = icon_loader_load;
    loader.unload = icon_loader_unload;
    loader.type_path = "icons";

    return loader;
}
