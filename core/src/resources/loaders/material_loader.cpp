#include "material_loader.hpp"
#include "core/logger.hpp"
#include "math/math.hpp"
#include "memory/memory.hpp"
#include "platform/filesystem.hpp"
#include "systems/resource_system.hpp"
#include "utils/string.hpp"

b8
material_loader_load(Arena      *arena,
                     const char *name,
                     Resource   *out_resource)
{
    if (!arena || !name || !out_resource)
    {
        CORE_ERROR(
            "material_loader_load - Ensure all pointers are not nullptr");
        return false;
    }

    String full_path = str_fmt(arena,
                               "%s/%s/%s%s",
                               resource_system_base_path(),
                               "materials",
                               name,
                               ".vol");

    out_resource->full_path = (char *)full_path.str;

    File_Handle file;
    if (!filesystem_open((const char *)full_path.str,
                         File_Modes::READ,
                         false,
                         &file))
    {
        CORE_ERROR(
            "material_loader_load - unable to open material file for "
            "reading: '%s'",
            (const char *)full_path.str);
        return false;
    }

    Material_Config *resource_data = push_struct(arena, Material_Config);
    // Set default values
    resource_data->auto_release    = true;
    resource_data->diffuse_color   = vec4_one();
    resource_data->diffuse_map_name = {};
    resource_data->name = const_str_from_cstr<MATERIAL_NAME_MAX_LENGTH>(name);

    char line_buffer[512] = "";
    char *p = &line_buffer[0];

    u64 line_length = 0;
    u32 line_number = 1;

    while (filesystem_read_line(&file, 511, &p, &line_length))
    {
        String trimmed = str_trim_whitespace(str_from_cstr(line_buffer));

        if (trimmed.size < 1 || trimmed.str[0] == '#')
        {
            line_number++;
            continue;
        }

        u64 equal_index = str_index_of(trimmed, '=');
        if (equal_index == (u64)-1)
        {
            CORE_WARN(
                "material_loader_load - Potential formatting issue found "
                "in '%s': token '=' not "
                "found. Skipping line '%ui'",
                (const char *)full_path.str,
                line_number);
            line_number++;
            continue;
        }

        String var_name = str_trim_whitespace(
            str_prefix(trimmed, equal_index));
        String value = str_trim_whitespace(
            str_skip(trimmed, equal_index + 1));

        if (str_match(var_name,
                      STR_LIT("version"),
                      String_Match_Flags::CASE_INSENSITIVE))
        {
            // TODO: handle version
        }
        else if (str_match(var_name,
                           STR_LIT("name"),
                           String_Match_Flags::CASE_INSENSITIVE))
        {
            resource_data->name =
                const_str_from_str<MATERIAL_NAME_MAX_LENGTH>(value);
        }
        else if (str_match(var_name,
                           STR_LIT("diffuse_map_name"),
                           String_Match_Flags::CASE_INSENSITIVE))
        {
            resource_data->diffuse_map_name =
                const_str_from_str<TEXTURE_NAME_MAX_LENGTH>(value);
        }
        else if (str_match(var_name,
                           STR_LIT("diffuse_color"),
                           String_Match_Flags::CASE_INSENSITIVE))
        {
            if (!str_to_vec4(value, &resource_data->diffuse_color))
            {
                CORE_WARN(
                    "material_loader_load - Error parsing diffuse color in "
                    "file '%s'. Using default "
                    "of white instead",
                    (const char *)full_path.str);
                resource_data->diffuse_color = vec4_one();
            }
        }

        memory_zero(line_buffer, sizeof(char) * 512);
        line_number++;
    }

    filesystem_close(&file);

    out_resource->data      = resource_data;
    out_resource->data_size = sizeof(Material_Config);
    out_resource->name      = name;

    return true;
}
