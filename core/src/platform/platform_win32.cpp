#include "defines.hpp"

#ifdef PLATFORM_WINDOWS

#    include "platform.hpp"
#    include <dwmapi.h>
#    include <windows.h>
#    pragma comment(lib, "dwmapi.lib")

extern "C" void platform_enable_rounded_corners(void *hwnd) {
    if (hwnd) {
        DWM_WINDOW_CORNER_PREFERENCE corner_preference = DWMWCP_ROUND;
        DwmSetWindowAttribute((HWND)hwnd,
            DWMWA_WINDOW_CORNER_PREFERENCE,
            &corner_preference,
            sizeof(corner_preference));
    }
}

void *platform_virtual_reserve(u64 size) {
    auto block = VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);

    return block;
}

void platform_virtual_commit(void *block, u64 size) {
    VirtualAlloc(block, size, MEM_COMMIT, PAGE_READWRITE);
}

void platform_virtual_release(void *block, u64 size) {
    VirtualFree(block, size, MEM_RELEASE);
}

void platform_virtual_decommit(void *block, u64 size) {
    VirtualFree(block, size, MEM_DECOMMIT);
}

u64 platform_query_page_size() {

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
}

#endif
