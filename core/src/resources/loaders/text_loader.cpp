#include "text_loader.hpp"
#include "core/logger.hpp"
#include "memory/memory.hpp"
#include "platform/filesystem.hpp"
#include "systems/resource_system.hpp"
#include "utils/string.hpp"

b8
text_loader_load(Arena      *arena,
                 const char *name,
                 Resource   *out_resource)
{
    if (!arena || !name || !out_resource)
    {
        CORE_ERROR("text_loader_load - Make sure all pointers are valid");
        return false;
    }

    String full_path = str_fmt(arena,
                               "%s/%s%s",
                               resource_system_base_path(),
                               name,
                               "");

    out_resource->full_path = (char *)full_path.str;

    File_Handle file;
    if (!filesystem_open((const char *)full_path.str,
                         File_Modes::READ,
                         false,
                         &file))
    {
        CORE_ERROR(
            "text_loader_load - unable to open text file for "
            "reading: '%s'",
            (const char *)full_path.str);
        return false;
    }

    u64 file_size = 0;
    if (!filesystem_size(&file, &file_size))
    {
        CORE_ERROR("text_loader_load - Unable to read file: '%s'",
                   (const char *)full_path.str);
        filesystem_close(&file);
        return false;
    }

    char *resource_data = push_array(arena, char, file_size + 1);

    u64 read_size = 0;
    if (!filesystem_read_all_text(&file, resource_data, &read_size))
    {
        CORE_ERROR("text_loader_load - Unable to read file as text '%s'",
                   (const char *)full_path.str);
        filesystem_close(&file);
        return false;
    }

    filesystem_close(&file);

    out_resource->data      = resource_data;
    out_resource->data_size = read_size;
    out_resource->name      = name;

    return true;
}
