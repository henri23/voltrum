#pragma once

#include "defines.hpp"

struct File_Handle {
    void* handle; // Handle to file can be different depending on the platform
    b8 is_valid;  // Used to check externally whether the file handle is valid
};

// Use c-style enum as oposed to enum classes to use a bit map for the case of
// read-write requests of handles
enum class File_Modes { READ, WRITE, READ_WRITE };

VOLTRUM_API b8 filesystem_exists(const char* path);

VOLTRUM_API b8 filesystem_open(const char* path,
    File_Modes mode,
    b8 binary,
    File_Handle* out_handle);

VOLTRUM_API void filesystem_close(File_Handle* handle);

// Used for text files, reads an entire line of text until it finds the \n or
// EOF characters
VOLTRUM_API b8 filesystem_read_line(File_Handle* handle,
    u64 max_length,
    char** line_buf,
    u64* out_line_length);

VOLTRUM_API b8 filesystem_write_line(File_Handle* handle, const char* text);

// Read certain number of bytes from a file
// out_data is the pointer to a block of memory of the data read from the file
// out_bytes_read is a pointer to the number of bytes read by this method
VOLTRUM_API b8 filesystem_read(File_Handle* handle,
    u64 data_size,
    void* out_data,
    u64* out_bytes_read);

// The out_bytes will be pointer to a byte array allocated and populated by this
// function
// out_bytes_read is a pointer to the number of bytes read from the file
VOLTRUM_API b8 filesystem_read_all_bytes(File_Handle* handle,
    u8** out_bytes,
    u64* out_bytes_read);

VOLTRUM_API b8 filesystem_write(File_Handle* handle,
    u64 data_size,
    const void* data,
    u64* out_bytes_written);
