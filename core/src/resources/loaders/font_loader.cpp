#include "font_loader.hpp"
#include "core/logger.hpp"
#include "memory/memory.hpp"
#include "platform/filesystem.hpp"
#include "systems/resource_system.hpp"
#include "utils/string.hpp"

b8
font_loader_load(Arena      *arena,
                 const char *name,
                 Resource   *out_resource)
{
    if (!arena || !name || !out_resource)
    {
        CORE_ERROR("font_loader_load - Make sure all pointers are valid");
        return false;
    }

    // Try .ttf first
    String full_path = string_fmt(arena,
                               "%s/%s/%s%s",
                               resource_system_base_path(),
                               "fonts",
                               name,
                               ".ttf");

    // If .ttf doesn't exist, try .otf
    if (!filesystem_exists((const char *)full_path.buff))
    {
        full_path = string_fmt(arena,
                            "%s/%s/%s%s",
                            resource_system_base_path(),
                            "fonts",
                            name,
                            ".otf");
    }

    out_resource->full_path = (char *)full_path.buff;

    File_Handle file;
    if (!filesystem_open((const char *)full_path.buff,
                         File_Modes::READ,
                         true,
                         &file))
    {
        CORE_ERROR(
            "font_loader_load - unable to open font file for "
            "reading: '%s'",
            (const char *)full_path.buff);
        return false;
    }

    u64 file_size = 0;
    if (!filesystem_size(&file, &file_size))
    {
        CORE_ERROR("font_loader_load - Unable to read file: '%s'",
                   (const char *)full_path.buff);
        filesystem_close(&file);
        return false;
    }

    u8 *resource_data = push_array(arena, u8, file_size);

    u64 read_size = 0;
    if (!filesystem_read_all_bytes(&file, resource_data, &read_size))
    {
        CORE_ERROR("font_loader_load - Unable to read font '%s'",
                   (const char *)full_path.buff);
        filesystem_close(&file);
        return false;
    }

    filesystem_close(&file);

    out_resource->data      = resource_data;
    out_resource->data_size = read_size;
    out_resource->name      = name;

    return true;
}
