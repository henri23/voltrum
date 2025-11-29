#include "text_loader.hpp"
#include "core/logger.hpp"
#include "memory/memory.hpp"
#include "platform/filesystem.hpp"
#include "utils/string.hpp"

INTERNAL_FUNC b8 text_resource_load(struct Resource_Loader* self,
    const char* name,
    Resource* out_resource) {
    if (!self || !name || !out_resource) {
        CORE_ERROR("text_resource_load - Make sure all pointers are valid");
        return false;
    }

    const char* format_str = "%s/%s/%s%s";
    char full_file_path[512];

    string_format(full_file_path,
        format_str,
        resource_system_base_path(),
        self->type_path,
        name,
        ""); // No need for extension because must be specified by the user

    out_resource->full_path = string_duplicate(full_file_path);

    File_Handle file;
    if (!filesystem_open(full_file_path, File_Modes::READ, false, &file)) {
        CORE_ERROR(
            "text_resource_load - unable to open text file for "
            "reading: '%s'",
            full_file_path);
        return false;
    }

    u64 file_size = 0;
    if (!filesystem_size(&file, &file_size)) {
        CORE_ERROR("text_loader_load - Unable to read file: '%s'",
            full_file_path);
        filesystem_close(&file);
        return false;
    }

    char* resource_data = static_cast<char*>(
        memory_allocate(sizeof(char) * file_size, Memory_Tag::ARRAY));

    u64 read_size = 0;
    if (!filesystem_read_all_text(&file, resource_data, &read_size)) {
        CORE_ERROR("text_loader_load - Unable to read file as text '%s'",
            full_file_path);
        filesystem_close(&file);
        return false;
    }

    filesystem_close(&file);

    out_resource->data = resource_data;
    out_resource->data_size = read_size;
    out_resource->name = name;

    return true;
}

INTERNAL_FUNC void text_resource_unload(struct Resource_Loader* self,
    Resource* resource) {

    if (!self || !resource) {
        CORE_WARN(
            "text_loader_unload called with nullptr for self of resource.");
        return;
    }

    u32 path_length = string_length(resource->full_path);
    memory_deallocate(resource->full_path,
        sizeof(char) * path_length,
        Memory_Tag::STRING);

    if (resource->data) {
        memory_deallocate(resource->data,
            resource->data_size,
            Memory_Tag::ARRAY);

        resource->data = nullptr;
        resource->data_size = 0;
        resource->loader_id = INVALID_ID;
    }
}

Resource_Loader text_resource_loader_create() {
    Resource_Loader loader = {};
    loader.load = text_resource_load;
    loader.unload = text_resource_unload;
    loader.type = Resource_Type::TEXT;
    loader.type_path = ""; // Similar binary loader, can be called from anywhere

    return loader;
}
