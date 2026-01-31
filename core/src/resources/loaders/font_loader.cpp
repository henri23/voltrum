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

    const char *format_str = "%s/%s/%s%s";
    char full_file_path[512];

    // Try .ttf first
    string_format(full_file_path,
                  format_str,
                  resource_system_base_path(),
                  "fonts",
                  name,
                  ".ttf");

    // If .ttf doesn't exist, try .otf
    if (!filesystem_exists(full_file_path))
    {
        string_format(full_file_path,
                      format_str,
                      resource_system_base_path(),
                      "fonts",
                      name,
                      ".otf");
    }

    // Arena-allocate full path copy
    u64 path_len = string_length(full_file_path);
    char *path_copy = push_array(arena, char, path_len + 1);
    memory_copy(path_copy, full_file_path, path_len + 1);
    out_resource->full_path = path_copy;

    File_Handle file;
    if (!filesystem_open(full_file_path, File_Modes::READ, true, &file))
    {
        CORE_ERROR(
            "font_loader_load - unable to open font file for "
            "reading: '%s'",
            full_file_path);
        return false;
    }

    u64 file_size = 0;
    if (!filesystem_size(&file, &file_size))
    {
        CORE_ERROR("font_loader_load - Unable to read file: '%s'",
                   full_file_path);
        filesystem_close(&file);
        return false;
    }

    u8 *resource_data = push_array(arena, u8, file_size);

    u64 read_size = 0;
    if (!filesystem_read_all_bytes(&file, resource_data, &read_size))
    {
        CORE_ERROR("font_loader_load - Unable to read font '%s'",
                   full_file_path);
        filesystem_close(&file);
        return false;
    }

    filesystem_close(&file);

    out_resource->data      = resource_data;
    out_resource->data_size = read_size;
    out_resource->name      = name;

    return true;
}
