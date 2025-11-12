cmake_minimum_required(VERSION 3.16)

project(voltrum)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_SUPPRESS_REGENERATION ON)

# Flexible linking option - default to static for better deployment
option(VOLTRUM_STATIC_LINKING "Link core and dependencies statically" ON)

# Configure external dependencies based on linking mode
if(VOLTRUM_STATIC_LINKING)
    message(
        STATUS
        "Voltrum: Static linking mode - all libraries will be statically linked"
    )

    # Force SDL3 to build static library only
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)

    # Force spdlog to build static library
    set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
else()
    message(
        STATUS
        "Voltrum: Dynamic linking mode - using shared libraries where available"
    )

    # Allow SDL3 to build shared library (default behavior)
    set(SDL_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "" FORCE)

    # Allow spdlog to build shared library
    set(SPDLOG_BUILD_SHARED ON CACHE BOOL "" FORCE)
endif()

# SDL3 configuration - Disable unused subsystems to speed up CMake configuration
set(SDL_AUDIO OFF CACHE BOOL "" FORCE) # disable audio subsystem
set(SDL_CAMERA OFF CACHE BOOL "" FORCE) # disable camera subsystem
set(SDL_JOYSTICK OFF CACHE BOOL "" FORCE) # disable joystick subsystem
set(SDL_HAPTIC OFF CACHE BOOL "" FORCE) # disable haptic subsystem
set(SDL_SENSOR OFF CACHE BOOL "" FORCE) # disable sensor subsystem
set(SDL_DIALOG OFF CACHE BOOL "" FORCE) # disable file dialog subsystem
set(SDL_TRAY OFF CACHE BOOL "" FORCE) # disable system tray subsystem
set(SDL_HIDAPI_LIBUSB OFF CACHE BOOL "" FORCE)
set(SDL_HIDAPI OFF CACHE BOOL "" FORCE)
set(SDL_LIBUSB OFF CACHE BOOL "" FORCE)
set(SDL_OPENGL OFF CACHE BOOL "" FORCE) # disable OpenGL (using Vulkan)
set(SDL_OPENGLES OFF CACHE BOOL "" FORCE) # disable OpenGL ES (using Vulkan)
set(SDL_RENDER OFF CACHE BOOL "" FORCE) # disable SDL render API (using Vulkan directly)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE) # don't install SDL system-wide
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE) # don't build SDL test library
set(SDL_TESTS OFF CACHE BOOL "" FORCE) # don't build SDL tests

option(VOLTRUM_BUILD_TESTS "Build Voltrum unit tests" ON)
if(VOLTRUM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# External dependencies
add_subdirectory(external/SDL3)
add_subdirectory(external/spdlog)

add_subdirectory(core)

add_subdirectory(client)
