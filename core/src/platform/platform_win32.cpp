#include "defines.hpp"

#ifdef PLATFORM_WINDOWS

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

#endif
