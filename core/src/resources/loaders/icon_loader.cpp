#include "icon_loader.hpp"
#include "core/logger.hpp"
#include "memory/memory.hpp"
#include "resources/resource_types.hpp"
#include "systems/resource_system.hpp"
#include "utils/string.hpp"

#include "stb_image.h"

b8
icon_loader_load(Arena      *arena,
                 const char *name,
                 Resource   *out_resource)
{
    if (!arena || !name || !out_resource)
    {
        CORE_ERROR("icon_loader_load - Ensure all pointers are not nullptr");
        return false;
    }

    const s32 required_channel_count = 4; // RGBA

    // Icons should NOT be flipped vertically (unlike textures)
    stbi_set_flip_vertically_on_load(false);

    String full_path = str_fmt(arena,
                               "%s/%s/%s%s",
                               resource_system_base_path(),
                               "icons",
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

    if (!data)
    {
        const char *fail_reason = stbi_failure_reason();
        CORE_ERROR("Icon resource loader failed to load file '%s': '%s'",
                   (const char *)full_path.str,
                   fail_reason ? fail_reason : "unknown error");
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

    CORE_DEBUG("Icon loaded: %s (%dx%d)", name, width, height);

    return true;
}
