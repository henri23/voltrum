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

    const char *format_str = "%s/%s/%s%s";
    char full_file_path[512];

    string_format(full_file_path,
                  format_str,
                  resource_system_base_path(),
                  "materials",
                  name,
                  ".vol");

    // Arena-allocate full path copy
    u64 path_len = string_length(full_file_path);
    char *path_copy = push_array(arena, char, path_len + 1);
    memory_copy(path_copy, full_file_path, path_len + 1);
    out_resource->full_path = path_copy;

    File_Handle file;
    if (!filesystem_open(full_file_path, File_Modes::READ, false, &file))
    {
        CORE_ERROR(
            "material_loader_load - unable to open material file for "
            "reading: '%s'",
            full_file_path);
        return false;
    }

    Material_Config *resource_data = push_struct(arena, Material_Config);
    // Set default values
    resource_data->auto_release = true;
    resource_data->diffuse_color = vec4_one();
    resource_data->diffuse_map_name[0] = '\0';
    string_ncopy(resource_data->name, name, MATERIAL_NAME_MAX_LENGTH);

    char line_buffer[512] = "";
    char *p = &line_buffer[0];

    u64 line_length = 0;
    u32 line_number = 1;

    while (filesystem_read_line(&file, 511, &p, &line_length))
    {
        char *trimmed = string_trim(line_buffer);

        line_length = string_length(trimmed);

        if (line_length < 1 || trimmed[0] == '#')
        {
            line_number++;
            continue;
        }

        s32 equal_index = string_index_of(trimmed, '=');
        if (equal_index == -1)
        {
            CORE_WARN(
                "material_loader_load - Potential formatting issue found "
                "in '%s': token '=' not "
                "found. Skipping line '%ui'",
                full_file_path,
                line_number);
            line_number++;
            continue;
        }

        char raw_var_name[64];
        memory_zero(raw_var_name, sizeof(char) * 64);
        string_substr(raw_var_name, trimmed, 0, equal_index);
        char *trimmed_var_name = string_trim(raw_var_name);

        // Assume a max line length of 511 - 64 (for the variable name)
        char raw_value[446];
        memory_zero(raw_value, sizeof(char) * 446);
        string_substr(raw_value,
                      trimmed,
                      equal_index + 1,
                      -1);
        char *trimmed_value = string_trim(raw_value);

        if (string_check_equal_insensitive(trimmed_var_name, "version"))
        {
            // TODO: handle version

        }
        else if (string_check_equal_insensitive(trimmed_var_name, "name"))
        {
            string_ncopy(resource_data->name,
                         trimmed_value,
                         MATERIAL_NAME_MAX_LENGTH);
        }
        else if (string_check_equal_insensitive(trimmed_var_name,
                                                "diffuse_map_name"))
        {
            string_ncopy(resource_data->diffuse_map_name,
                         trimmed_value,
                         TEXTURE_NAME_MAX_LENGTH);
        }
        else if (string_check_equal_insensitive(trimmed_var_name,
                                                "diffuse_color"))
        {
            if (!string_to_vec4(trimmed_value, &resource_data->diffuse_color))
            {
                CORE_WARN(
                    "material_loader_load - Error parsing diffuse color in "
                    "file '%s'. Using default "
                    "of white instead",
                    full_file_path);
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
