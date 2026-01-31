#include "image_loader.hpp"
#include "core/asserts.hpp"
#include "core/logger.hpp"
#include "memory/memory.hpp"
#include "resources/resource_types.hpp"
#include "systems/resource_system.hpp"
#include "utils/string.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

b8
image_loader_load(Arena      *arena,
                  const char *name,
                  Resource   *out_resource)
{
    if (!arena || !name || !out_resource)
    {
        CORE_ERROR("image_loader_load - Ensure all pointers are not nullptr");
        return false;
    }

    const s32 required_channel_count = 4; // RGBA

    // NOTE: Most images are stored in a format where the data is actually
    // stored upside-down, so if we load the data from the bottom -> up we
    // should technically retrieve the original orientation
    stbi_set_flip_vertically_on_load(true);

    String full_path = str_fmt(arena,
                               "%s/%s/%s%s",
                               resource_system_base_path(),
                               "textures",
                               name,
                               ".png");

    s32 width;
    s32 height;
    s32 channel_count;

    u8 *data = stbi_load((const char *)full_path.str,
                         &width,
                         &height,
                         &channel_count,
                         required_channel_count);

    const char *fail_reason = stbi_failure_reason();
    if (fail_reason)
    {
        CORE_ERROR("Image resource loader failed to load file '%s': '%s'",
                   (const char *)full_path.str,
                   fail_reason);

        stbi__err(0, 0);

        if (data)
        {
            stbi_image_free(data);
        }

        return false;
    }

    if (!data)
    {
        CORE_ERROR("Image resource loader failed to load file '%s'",
                   (const char *)full_path.str);
        return false;
    }

    u32 pixel_data_size = width * height * required_channel_count;

    // Copy stbi data into arena
    u8 *pixels = push_array(arena, u8, pixel_data_size);
    memory_copy(pixels, data, pixel_data_size);

    // Free stbi allocated memory immediately
    stbi_image_free(data);

    out_resource->full_path = (char *)full_path.str;

    Image_Resource_Data *resource_data =
        push_struct(arena, Image_Resource_Data);

    resource_data->pixels        = pixels;
    resource_data->width         = width;
    resource_data->height        = height;
    resource_data->channel_count = required_channel_count;

    out_resource->data      = resource_data;
    out_resource->data_size = sizeof(Image_Resource_Data);
    out_resource->name      = name;

    return true;
}
