cmake_minimum_required(VERSION 3.16)
project(voltrum_core)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(VULKAN_SDK $ENV{VULKAN_SDK})

# Set linking mode preprocessor definitions (option defined in root CMakeLists.txt)
if(VOLTRUM_STATIC_LINKING)
    add_compile_definitions(VOLTRUM_STATIC_LINKING)
    message(STATUS "Voltrum Core: Static linking enabled")
else()
    add_compile_definitions(API_EXPORT)
    message(STATUS "Voltrum Core: Dynamic linking enabled")
endif()

# Set compiler flags based on build type
if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_definitions(DEBUG_BUILD)
    if(MSVC)
        # MSVC Debug flags
        add_compile_options(/Od /Zi /RTC1)
    else()
        # GCC/Clang Debug flags
        add_compile_options(-O0 -g -fno-omit-frame-pointer)
    endif()
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    add_compile_definitions(RELEASE_BUILD)
    if(MSVC)
        # MSVC Release flags
        add_compile_options(/O2)
    else()
        # GCC/Clang Release flags
        add_compile_options(-O2)
    endif()
endif()

# Windows/MSVC specific settings
if(WIN32)
    # Disable some MSVC warnings
    if(MSVC)
        add_compile_options(/W4) # High warning level
        add_compile_definitions(_CRT_SECURE_NO_WARNINGS) # Disable security warnings for standard C functions
        add_compile_definitions(NOMINMAX) # Prevent windows.h from defining min/max macros
        add_compile_definitions(WIN32_LEAN_AND_MEAN) # Reduce Windows header bloat
    endif()
endif()

file(GLOB_RECURSE ENGINE_SRC "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.hpp")

# Exclude all ui/*.cpp files from build (commenting out for UI rewrite)
# Keep vulkan_ui_backend.cpp which is in renderer/vulkan/ui_backend/
list(FILTER ENGINE_SRC EXCLUDE REGEX "src/ui/ui.*\\.cpp$")

message(STATUS "Core sources found: ${ENGINE_SRC}")
message(STATUS "Core headers found: ${HEADERS}")

find_package(Vulkan REQUIRED)

# Create the voltrum core library (static or shared based on option)
if(VOLTRUM_STATIC_LINKING)
    add_library(${PROJECT_NAME} STATIC ${ENGINE_SRC} ${HEADERS})
else()
    add_library(${PROJECT_NAME} SHARED ${ENGINE_SRC} ${HEADERS})
endif()

# Link libraries (Vulkan and SDL3)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
target_link_libraries(${PROJECT_NAME} PUBLIC spdlog::spdlog)

# Include directories
target_include_directories(${PROJECT_NAME} PUBLIC "src")
target_include_directories(${PROJECT_NAME} PRIVATE ${VULKAN_SDK}/Include)
target_include_directories(
    ${PROJECT_NAME}
    PRIVATE ${CMAKE_SOURCE_DIR}/external/stb_image
)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/assets)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)

set(IMGUI_SRC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_demo.cpp # optional
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

add_library(imgui STATIC ${IMGUI_SRC})
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(imgui PUBLIC SDL3::SDL3 Vulkan::Vulkan)

# Link imgui into core (PUBLIC so client can access ImGui headers)
target_link_libraries(${PROJECT_NAME} PUBLIC imgui)
