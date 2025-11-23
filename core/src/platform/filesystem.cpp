#include "filesystem.hpp"

#include "core/logger.hpp"
#include "memory/memory.hpp"

#include <stdio.h>
#include <string.h>

#ifdef PLATFORM_WINDOWS
#    include <io.h>
#    include <sys/stat.h>
#    include <sys/types.h>
#else
#    include <sys/stat.h>
#endif

// TODO: 	Refactor this to not use the standart library but instead the
// native 			filesystems of the platforms
b8 filesystem_exists(const char* path) {
#ifdef PLATFORM_WINDOWS
    struct _stat buffer;
    return _stat(path, &buffer);
#else
    struct stat buffer;
    return stat(path, &buffer) == 0;
#endif
}

b8 filesystem_open(
    const char* path,
    File_Modes mode,
    b8 binary,
    File_Handle* out_handle
) {

    out_handle->is_valid = false;
    out_handle->handle = nullptr;
    const char* mode_str;

    b8 read_mode = static_cast<u8>(mode & File_Modes::READ) != 0;
    b8 write_mode = static_cast<u8>(mode & File_Modes::WRITE) != 0;

    if (read_mode && write_mode) {
        mode_str = binary ? "w+b" : "w+";
    } else if (read_mode) {
        mode_str = binary ? "rb" : "r";
    } else if (write_mode) {
        mode_str = binary ? "wb" : "w";
    } else {
        CORE_ERROR("Invalid mode passed while trying to open file: '%s'", path);

        return false;
    }

    FILE* file = fopen(path, mode_str);

    if (!file) {
        CORE_ERROR("Error while opening file: '%s'", path);
        return false;
    }

    out_handle->handle = file;
    out_handle->is_valid = true;

    return true;
}

void filesystem_close(File_Handle* handle) {
    if (handle->handle) {
        fclose(static_cast<FILE*>(handle->handle));
        handle->handle = nullptr;
        handle->is_valid = false;
    }
}

// line_buf char array will be allocated inside the method. No need for external
// memory management
b8 filesystem_read_line(File_Handle* handle,
    u64 max_length,
    char** line_buf,
    u64* out_line_length) {

    if (handle->handle && line_buf && out_line_length && max_length > 0) {
        char* buffer = *line_buf;
        // fgets stops reading when it encounters a /n, EOF or has read num - 1
        // where in this case num = max_length
        // fgets returns the same pointer that is passed to it as the first arg
        // in case of success and it returns NULL when it encounters an error or
        // EOF
        if (fgets(buffer, max_length, (FILE*)handle->handle) != nullptr) {
            *out_line_length = strlen(*line_buf);
            return true;
        }
    }

    return false;
}

b8 filesystem_write_line(File_Handle* handle, const char* text) {

    if (handle->handle) {
        s32 result = fputs(text, static_cast<FILE*>(handle->handle));
        if (result != EOF) {
            fputc('\n', static_cast<FILE*>(handle->handle));
        }

        // In order to write the line immediatelly to the file stream, we need
        // to flush the stream. This will prevent possible data loss in the
        // event of a freeze
        fflush(static_cast<FILE*>(handle->handle));
        return result != EOF;
    }

    return false;
}

// Read certain number of bytes from a file
// out_data is the pointer to a block of memory of the data read from the file
// out_bytes_read is a pointer to the number of bytes read by this method
b8 filesystem_read(File_Handle* handle,
    u64 data_size,
    void* out_data,
    u64* out_bytes_read) {

    if (handle->handle && out_data) {
        *out_bytes_read =
            fread(out_data, 1, data_size, static_cast<FILE*>(handle->handle));

        if (*out_bytes_read != data_size) {
            return false;
        }

        return true;
    }

    return false;
}

// The out_bytes will be pointer to a byte array allocated and populated by this
// function
// out_bytes_read is a pointer to the number of bytes read from the file
b8 filesystem_read_all_bytes(File_Handle* handle,
    u8** out_bytes,
    u64* out_bytes_read) {

    if (handle->handle) {
        // This method does not preserve the file pointer, which in this case is
        // ok because I just want to read the whole file in this method

        // fseek moves the file pointer from the current position (in this case
        // at the beginning of the file because it was just opened) to the
        // specified position declared in the 3rd argument
        fseek(static_cast<FILE*>(handle->handle), 0, SEEK_END);

        u64 size = ftell(static_cast<FILE*>(handle->handle));

        // Rewind resset the pointer to the beginning
        rewind(static_cast<FILE*>(handle->handle));

        *out_bytes =
            static_cast<u8*>(memory_allocate(size, Memory_Tag::STRING));

        *out_bytes_read =
            fread(*out_bytes, 1, size, static_cast<FILE*>(handle->handle));

        if (*out_bytes_read != size)
            return false;

        return true;
    }

    return false;
}

b8 filesystem_write(File_Handle* handle,
    u64 data_size,
    const void* data,
    u64* out_bytes_written) {

    if (handle->handle) {
        *out_bytes_written =
            fwrite(data, 1, data_size, static_cast<FILE*>(handle->handle));

        if (*out_bytes_written != data_size) {
            return false;
        }
        fflush(static_cast<FILE*>(handle->handle));
        return true;
    }
    return false;
}
