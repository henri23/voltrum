#include "defines.hpp"

#ifdef PLATFORM_WINDOWS

// clang-format off
#    include <WinSock2.h>
#    include <MSWSock.h>
// clang-format on
#    include <dwmapi.h>
#    pragma comment(lib, "dwmapi.lib")

#    include "core/asserts.hpp"
#    include "platform.hpp"

typedef HRESULT W32_SetThreadDescription_Type(HANDLE hThread,
    PCWSTR lpThreadDescription);

global_variable W32_SetThreadDescription_Type *w32_SetThreadDescription_func;
global_variable RIO_EXTENSION_FUNCTION_TABLE w32_rio_functions;

C_LINKAGE void
platform_enable_rounded_corners(void *hwnd) {
    if (hwnd) {
        DWM_WINDOW_CORNER_PREFERENCE corner_preference = DWMWCP_ROUND;
        DwmSetWindowAttribute((HWND)hwnd,
            DWMWA_WINDOW_CORNER_PREFERENCE,
            &corner_preference,
            sizeof(corner_preference));
    }
}

void *
platform_virtual_memory_reserve(u64 size) {
    void *block = VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE);

    return block;
}

b8
platform_virtual_memory_commit(void *block, u64 size) {
    // When committing memory, the block is marked to be committed, but it will
    // be done so on the first page fault. It is not actually backed
    // immediatelly by physical memory
    b8 result = (VirtualAlloc(block, size, MEM_COMMIT, PAGE_READWRITE) != 0);

    // The RIORegisterBuffer function creates a registered buffer identifier for
    // a specified buffer. When a buffer is registered, the virtual memory pages
    // containing the buffer will be locked into physical memory.
    // Source:
    // https://learn.microsoft.com/en-us/previous-versions/windows/desktop/legacy/hh437199(v=vs.85)
    // w32_rio_functions.RIODeregisterBuffer(
    //     w32_rio_functions.RIORegisterBuffer(static_cast<PCHAR>(block),
    //     size));

    return result;
}

void
platform_virtual_memory_release(void *block, u64 size) {
    // Size not used because Windows does not need size to release the memory
    (void)size;
    VirtualFree(block, 0, MEM_RELEASE);
}

void
platform_virtual_memory_decommit(void *block, u64 size) {
    VirtualFree(block, size, MEM_DECOMMIT);
}

Platform_System_Info
platform_query_system_info() {

    SYSTEM_INFO si = {};
    GetSystemInfo(&si);

    Platform_System_Info system_info = {};
    system_info.logical_processor_count = (u32)si.dwNumberOfProcessors;
    system_info.page_size = si.dwPageSize;
    // system_info.large_page_size = (u32)si.dwNumberOfProcessors;
    system_info.allocation_granularity = si.dwAllocationGranularity;

    return system_info;
}

#endif
