#include "defines.hpp"

#ifdef PLATFORM_WINDOWS

#include "platform.hpp"
#include <dwmapi.h>
#include <windows.h>
#pragma comment(lib, "dwmapi.lib")

extern "C" void platform_enable_rounded_corners(void* hwnd) {
    if (hwnd) {
        DWM_WINDOW_CORNER_PREFERENCE corner_preference = DWMWCP_ROUND;
        DwmSetWindowAttribute((HWND)hwnd,
            DWMWA_WINDOW_CORNER_PREFERENCE,
            &corner_preference,
            sizeof(corner_preference));
    }
}

Virtual_Memory_Block platform_virtual_reserve(u64 size) {
    Virtual_Memory_Block block = {};
    // MEM_RESERVE | MEM_COMMIT: OS lazily backs pages on first access
    // Pages are guaranteed zero-initialized by Windows
    block.base = VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (block.base) {
        block.size = size;
    }
    return block;
}

void platform_virtual_release(Virtual_Memory_Block* block) {
    if (block && block->base) {
        VirtualFree(block->base, 0, MEM_RELEASE);
        block->base = nullptr;
        block->size = 0;
    }
}

u64 platform_get_page_size() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
}

#endif
