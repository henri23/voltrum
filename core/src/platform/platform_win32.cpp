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
    // MEM_RESERVE | MEM_COMMIT: OS lazily backs pages on first access
    // Pages are guaranteed zero-initialized by Windows
    auto block =
        VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    return block;
}

void platform_virtual_release(void *block, u64 size) {
    (void)size; // Windows ignores size with MEM_RELEASE
    VirtualFree(block, 0, MEM_RELEASE);
}

u64 platform_query_page_size() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
}

#endif
