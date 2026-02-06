#include "platform.hpp"

#include <stdlib.h>
#include <string.h>

#include "core/frame_context.hpp"
#include "core/logger.hpp"
#include "data_structures/dynamic_array.hpp"
#include "events/events.hpp"
#include "renderer/vulkan/vulkan_types.hpp"

#include "input/input.hpp"
#include "input/input_codes.hpp"

#include <SDL3/SDL_vulkan.h>
#include <imgui_impl_sdl3.h>

#ifdef PLATFORM_WINDOWS
extern "C" void platform_enable_rounded_corners(void *hwnd);
#endif

internal_var Platform_State *state_ptr = nullptr;

// Hit test callback for native window dragging/resizing
INTERNAL_FUNC SDL_HitTestResult
platform_hit_test_callback(SDL_Window *win, const SDL_Point *area, void *data);

Platform_State *
platform_init(Arena *allocator, String application_name, s32 width, s32 height)
{

    Platform_State *state =
        (Platform_State *)push_struct(allocator, Platform_State);

    state_ptr = state;

    CORE_DEBUG("Starting platform subsystem...");

#ifdef PLATFORM_LINUX
    // Set to true to force X11 for testing
    constexpr b8 force_x11 = false;

    if (force_x11)
    {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11");
        CORE_DEBUG("Forcing X11 video driver for testing");
    }
    else
    {
        // Prefer Wayland over X11 if Wayland is available
        // SDL will automatically fall back to X11 if Wayland is not available
        const char *wayland_display = getenv("WAYLAND_DISPLAY");
        if (wayland_display && wayland_display[0] != '\0')
        {
            SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland,x11");
            CORE_DEBUG(
                "Wayland detected, preferring Wayland video driver with X11 "
                "fallback");
        }
        else
        {
            CORE_DEBUG("Wayland not detected, using default X11 video driver");
        }
    }
#endif

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        CORE_ERROR("SDL_Init() failed with message:'%s'", SDL_GetError());

        return nullptr;
    }

    CORE_DEBUG("SDL initialized successfully");

    // Create window with Vulkan graphics context
    SDL_DisplayID primary_display = SDL_GetPrimaryDisplay();

    // Clamp window size to fit within the usable display area. On macOS,
    // logical display resolutions are often smaller than on Windows/Linux
    // (e.g. 1440x900 on a MacBook Air), so the requested size may exceed
    // the available space.
    SDL_Rect usable_bounds;
    if (SDL_GetDisplayUsableBounds(primary_display, &usable_bounds))
    {
        s32 max_w = (s32)(usable_bounds.w * 0.85f);
        s32 max_h = (s32)(usable_bounds.h * 0.85f);
        width     = CLAMP_TOP(width, max_w);
        height    = CLAMP_TOP(height, max_h);
    }

    SDL_WindowFlags window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE |
                                   SDL_WINDOW_HIGH_PIXEL_DENSITY |
                                   SDL_WINDOW_BORDERLESS;

    state->window = SDL_CreateWindow((const char *)application_name.str,
                                     width,
                                     height,
                                     window_flags);

    if (state->window == nullptr)
    {
        CORE_ERROR("SDL_CreateWindow() failed with message: '%s'",
                   SDL_GetError());

        return nullptr;
    }

    // Compute DPI scale from actual window pixel ratio. SDL_GetDisplayContentScale
    // can return 1.0 on macOS even for Retina displays, so we derive it from the
    // window's physical vs logical size which is always correct.
    int logical_w, pixel_w;
    SDL_GetWindowSize(state->window, &logical_w, nullptr);
    SDL_GetWindowSizeInPixels(state->window, &pixel_w, nullptr);
    state->main_scale = (logical_w > 0) ? (f32)pixel_w / (f32)logical_w : 1.0f;
    CORE_DEBUG("Window created successfully (DPI scale: %.2f)", state->main_scale);

#ifdef PLATFORM_WINDOWS
    // Enable Windows 11 rounded corners for borderless window
    void *hwnd = SDL_GetPointerProperty(SDL_GetWindowProperties(state->window),
                                        SDL_PROP_WINDOW_WIN32_HWND_POINTER,
                                        nullptr);
    platform_enable_rounded_corners(hwnd);
    CORE_DEBUG("Windows 11 rounded corners enabled");
#endif

    // Enable native window dragging and resizing for borderless window
    bool hit_test_result = SDL_SetWindowHitTest(state->window,
                                                platform_hit_test_callback,
                                                nullptr);

    if (hit_test_result)
    {
        CORE_DEBUG("SDL hit test callback registered successfully");
    }
    else
    {
        CORE_ERROR("Failed to register SDL hit test callback: %s",
                   SDL_GetError());
    }

    SDL_SetWindowPosition(state->window,
                          SDL_WINDOWPOS_CENTERED,
                          SDL_WINDOWPOS_CENTERED);

    SDL_ShowWindow(state->window);

    CORE_DEBUG("Window positioned and shown");
    CORE_INFO("Platform subsystem initialized successfully");

    return state;
}

void
platform_shutdown(Platform_State *state)
{
    CORE_DEBUG("Platform shutting down...");

    if (state != nullptr && state->window)
    {
        SDL_DestroyWindow(state->window);
        state->window = nullptr;
    }

    SDL_Quit();
    CORE_DEBUG("Platform shut down.");
}

b8
platform_message_pump(Frame_Context *frame_ctx)
{
    SDL_Event sdl_event;
    Event     engine_event = {};

    b8 quit_flagged = false;

    while (SDL_PollEvent(&sdl_event))
    {

        SDL_Keymod    sdl_mods  = SDL_GetModState();
        Key_Modifiers modifiers = Key_Modifiers::NONE;

        if (sdl_mods & (SDL_KMOD_LSHIFT | SDL_KMOD_RSHIFT))
        {
            modifiers |= Key_Modifiers::SHIFT;
        }
        if (sdl_mods & (SDL_KMOD_LCTRL | SDL_KMOD_RCTRL))
        {
            modifiers |= Key_Modifiers::CTRL;
        }
        if (sdl_mods & (SDL_KMOD_LALT | SDL_KMOD_RALT))
        {
            modifiers |= Key_Modifiers::ALT;
        }

        // TODO: Maybe use the event priority system to process events from the
        // event dispatcher
        ImGui_ImplSDL3_ProcessEvent(&sdl_event);

        switch (sdl_event.type)
        {
        case SDL_EVENT_QUIT:
            quit_flagged = true;
            break;

        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            quit_flagged      = true;
            engine_event.type = Event_Type::WINDOW_CLOSED;
            event_queue_produce(frame_ctx->event_queue, engine_event);
            break;

        case SDL_EVENT_KEY_DOWN:
            engine_event.type = Event_Type::KEY_PRESSED;
            engine_event.key.key_code =
                platform_to_key_code(sdl_event.key.scancode);
            engine_event.key.repeat    = sdl_event.key.repeat;
            engine_event.key.modifiers = modifiers;
            event_queue_produce(frame_ctx->event_queue, engine_event);
            input_process_key(platform_to_key_code(sdl_event.key.scancode),
                              true);
            break;

        case SDL_EVENT_KEY_UP:
            engine_event.type = Event_Type::KEY_RELEASED;
            engine_event.key.key_code =
                platform_to_key_code(sdl_event.key.scancode);
            engine_event.key.repeat    = false;
            engine_event.key.modifiers = modifiers;
            event_queue_produce(frame_ctx->event_queue, engine_event);
            input_process_key(platform_to_key_code(sdl_event.key.scancode),
                              false);
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            engine_event.type = Event_Type::MOUSE_BUTTON_PRESSED;
            engine_event.mouse_button.button =
                platform_to_mouse_button(sdl_event.button.button);
            engine_event.mouse_button.x         = (s32)sdl_event.button.x;
            engine_event.mouse_button.y         = (s32)sdl_event.button.y;
            engine_event.mouse_button.modifiers = modifiers;
            event_queue_produce(frame_ctx->event_queue, engine_event);
            input_process_mouse_button(
                platform_to_mouse_button(sdl_event.button.button),
                true);
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            engine_event.type = Event_Type::MOUSE_BUTTON_RELEASED;
            engine_event.mouse_button.button =
                platform_to_mouse_button(sdl_event.button.button);
            engine_event.mouse_button.x         = (s32)sdl_event.button.x;
            engine_event.mouse_button.y         = (s32)sdl_event.button.y;
            engine_event.mouse_button.modifiers = modifiers;
            event_queue_produce(frame_ctx->event_queue, engine_event);
            input_process_mouse_button(
                platform_to_mouse_button(sdl_event.button.button),
                false);
            break;

        case SDL_EVENT_MOUSE_MOTION:
            engine_event.type               = Event_Type::MOUSE_MOVED;
            engine_event.mouse_move.x       = (s32)sdl_event.motion.x;
            engine_event.mouse_move.y       = (s32)sdl_event.motion.y;
            engine_event.mouse_move.delta_x = (s32)sdl_event.motion.xrel;
            engine_event.mouse_move.delta_y = (s32)sdl_event.motion.yrel;
            event_queue_produce(frame_ctx->event_queue, engine_event);
            input_process_mouse_move((s32)sdl_event.motion.x,
                                     (s32)sdl_event.motion.y);
            break;

        case SDL_EVENT_MOUSE_WHEEL:
            engine_event.type                = Event_Type::MOUSE_WHEEL_SCROLLED;
            engine_event.mouse_wheel.x       = (s32)sdl_event.wheel.mouse_x;
            engine_event.mouse_wheel.y       = (s32)sdl_event.wheel.mouse_y;
            engine_event.mouse_wheel.delta_x = sdl_event.wheel.x;
            engine_event.mouse_wheel.delta_y = sdl_event.wheel.y;
            event_queue_produce(frame_ctx->event_queue, engine_event);
            input_process_mouse_wheel(sdl_event.wheel.x, sdl_event.wheel.y);
            break;

        case SDL_EVENT_WINDOW_RESIZED:
            engine_event.type = Event_Type::WINDOW_RESIZED;
            platform_get_drawable_size(&engine_event.window_resize.width,
                                       &engine_event.window_resize.height);
            event_queue_produce(frame_ctx->event_queue, engine_event);
            break;

        case SDL_EVENT_WINDOW_MINIMIZED:
            engine_event.type = Event_Type::WINDOW_MINIMIZED;
            event_queue_produce(frame_ctx->event_queue, engine_event);
            break;

        case SDL_EVENT_WINDOW_MAXIMIZED:
            engine_event.type = Event_Type::WINDOW_MAXIMIZED;
            event_queue_produce(frame_ctx->event_queue, engine_event);
            break;

        case SDL_EVENT_WINDOW_RESTORED:
            engine_event.type = Event_Type::WINDOW_RESTORED;
            event_queue_produce(frame_ctx->event_queue, engine_event);
            break;

        default:
            break;
        }
    }

    return !quit_flagged;
}

void
platform_get_vulkan_extensions(Dynamic_Array<const char *> *extensions)
{
    uint32_t sdl_extensions_count = 0;

    const char *const *sdl_extensions =
        SDL_Vulkan_GetInstanceExtensions(&sdl_extensions_count);

    for (uint32_t n = 0; n < sdl_extensions_count; n++)
        extensions->add(sdl_extensions[n]);
}

b8
platform_create_vulkan_surface(Vulkan_Context *context,
                               Platform_State *plat_state)
{

    if (!plat_state)
        return false;

    if (SDL_Vulkan_CreateSurface(plat_state->window,
                                 context->instance,
                                 context->allocator,
                                 &context->surface) == 0)
    {
        CORE_ERROR("Failed to create Vulkan surface.");
        return false;
    }

    return true;
}

void *
platform_allocate(u64 size, b8 aligned)
{
    return malloc(size);
}

void
platform_free(void *block, b8 aligned)
{
    free(block);
}

void *
platform_zero_memory(void *block, u64 size)
{
    return memset(block, 0, size);
}

void *
platform_copy_memory(void *dest, const void *source, u64 size)
{
    return memcpy(dest, source, size);
}

void *
platform_move_memory(void *dest, const void *source, u64 size)
{
    return memmove(dest, source, size);
}

void *
platform_set_memory(void *dest, s32 value, u64 size)
{
    return memset(dest, value, size);
}

f64
platform_get_absolute_time()
{
    return (f64)SDL_GetTicksNS() /
           1000000000.0; // Convert nanoseconds to seconds
}

void
platform_sleep(u64 ms)
{
    SDL_Delay((u32)ms);
}

void
platform_get_drawable_size(u32 *width, u32 *height)
{
    if (!state_ptr || !state_ptr->window)
    {
        *width  = 0;
        *height = 0;
        return;
    }

    int w, h;
    SDL_GetWindowSizeInPixels(state_ptr->window, &w, &h);

    CORE_DEBUG("platform_get_drawable_size: (%d:%d) in physical pixels", w, h);

    *width  = (u32)w;
    *height = (u32)h;
}

// TODO: Publish an event that is captured in the application file instead of
// calling directly from ui_titlebar
void
platform_minimize_window(Platform_State *state)
{
    if (state && state->window)
    {
        SDL_MinimizeWindow(state->window);
        CORE_DEBUG("Window minimized");
    }
}

void
platform_maximize_window(Platform_State *state)
{
    if (state && state->window)
    {
        SDL_MaximizeWindow(state->window);
        CORE_DEBUG("Window maximized");
    }
}

void
platform_restore_window(Platform_State *state)
{
    if (state && state->window)
    {
        SDL_RestoreWindow(state->window);
        CORE_DEBUG("Window restored");
    }
}

void
platform_close_window()
{
    if (state_ptr && state_ptr->window)
    {
        SDL_Event quit_event;
        quit_event.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&quit_event);
        CORE_DEBUG("Window close requested");
    }
}

b8
platform_is_window_maximized(Platform_State *state)
{
    if (state && state->window)
    {
        SDL_WindowFlags flags = SDL_GetWindowFlags(state->window);
        return (flags & SDL_WINDOW_MAXIMIZED) != 0;
    }
    return false;
}

void
platform_set_window_icon(Platform_State *state,
                         u8             *pixels,
                         u32             width,
                         u32             height)
{
    if (!state || !state->window || !pixels)
    {
        CORE_WARN("platform_set_window_icon: Invalid parameters");
        return;
    }

    SDL_Surface *icon_surface = SDL_CreateSurfaceFrom(width,
                                                      height,
                                                      SDL_PIXELFORMAT_RGBA32,
                                                      pixels,
                                                      width * 4);

    if (icon_surface)
    {
        SDL_SetWindowIcon(state->window, icon_surface);
        SDL_DestroySurface(icon_surface);

        CORE_DEBUG("Window icon set successfully (%dx%d)", width, height);
    }
    else
    {
        CORE_WARN("Failed to create SDL surface for icon: %s", SDL_GetError());
    }
}

void
platform_get_required_extensions(
    Dynamic_Array<const char *> *required_extensions)
{
    // Get the extensions needed by SDL3 for Vulkan
    Uint32             extension_count = 0;
    const char *const *extensions =
        SDL_Vulkan_GetInstanceExtensions(&extension_count);

    if (!extensions)
    {
        CORE_ERROR("Failed to get Vulkan instance extensions from SDL3: %s",
                   SDL_GetError());
        return;
    }

    // Add all required extensions to the array
    for (Uint32 i = 0; i < extension_count; ++i)
    {
        required_extensions->add(extensions[i]);
        CORE_DEBUG("Required Vulkan extension: %s", extensions[i]);
    }

#ifdef PLATFORM_APPLE
    // Add macOS specific portability extension required by MoltenVK
    required_extensions->add("VK_KHR_portability_enumeration");
    required_extensions->add("VK_KHR_get_physical_device_properties2");

    CORE_DEBUG("Added macOS portability extensions for MoltenVK");
#endif

    CORE_DEBUG("Added %u Vulkan extensions from SDL3", extension_count);
}

INTERNAL_FUNC SDL_HitTestResult
platform_hit_test_callback(SDL_Window *win, const SDL_Point *area, void *data)
{

    int window_width, window_height;
    SDL_GetWindowSize(win, &window_width, &window_height);

    // Check if we're in titlebar drag area first (should work even when
    // maximized). On macOS, SDL's Cocoa backend passes logical (point)
    // coordinates to the hit test callback, so no DPI scaling is needed.
    // On Linux/Windows, hit test coordinates are in physical pixels, so we
    // scale by the DPI factor.
    const int TITLEBAR_HEIGHT_LOGICAL = 58;
#ifdef PLATFORM_APPLE
    const int TITLEBAR_HEIGHT_THRESHOLD = TITLEBAR_HEIGHT_LOGICAL;
#else
    int window_width_pixels, window_height_pixels;
    SDL_GetWindowSizeInPixels(win, &window_width_pixels, &window_height_pixels);
    f32       scale_y = (f32)window_height_pixels / window_height;
    const int TITLEBAR_HEIGHT_THRESHOLD =
        (int)(TITLEBAR_HEIGHT_LOGICAL * scale_y);
#endif

    if (area->y <= TITLEBAR_HEIGHT_THRESHOLD)
    {
        // Check if mouse is over window control buttons - let ImGui handle clicks
        if (state_ptr->button_area_max_x > 0)
        {
            f32 px = (f32)area->x;
            f32 py = (f32)area->y;

            if (px >= state_ptr->button_area_min_x &&
                px <= state_ptr->button_area_max_x &&
                py >= state_ptr->button_area_min_y &&
                py <= state_ptr->button_area_max_y)
            {
                return SDL_HITTEST_NORMAL;
            }
        }

        // Let ImGui handle the input if an ImGui window overlaps the
        // titlebar area (e.g. a floating window dragged over it)
        if (state_ptr->block_titlebar_drag)
        {
            return SDL_HITTEST_NORMAL;
        }
        return SDL_HITTEST_DRAGGABLE;
    }

    // Don't allow resizing when window is maximized, but allow titlebar
    // dragging above
    if (platform_is_window_maximized(state_ptr))
    {
        return SDL_HITTEST_NORMAL;
    }

    const int BORDER_SIZE = 4; // Size of resize borders

    // Check for resize areas (edges and corners) - only when not maximized
    bool on_left   = area->x <= BORDER_SIZE;
    bool on_right  = area->x >= window_width - BORDER_SIZE;
    bool on_top    = area->y <= BORDER_SIZE;
    bool on_bottom = area->y >= window_height - BORDER_SIZE;

    // Corner resizing (higher priority)
    if (on_top && on_left)
        return SDL_HITTEST_RESIZE_TOPLEFT;
    if (on_top && on_right)
        return SDL_HITTEST_RESIZE_TOPRIGHT;
    if (on_bottom && on_left)
        return SDL_HITTEST_RESIZE_BOTTOMLEFT;
    if (on_bottom && on_right)
        return SDL_HITTEST_RESIZE_BOTTOMRIGHT;

    // Edge resizing
    if (on_top)
        return SDL_HITTEST_RESIZE_TOP;
    if (on_bottom)
        return SDL_HITTEST_RESIZE_BOTTOM;
    if (on_left)
        return SDL_HITTEST_RESIZE_LEFT;
    if (on_right)
        return SDL_HITTEST_RESIZE_RIGHT;

    return SDL_HITTEST_NORMAL;
}
