cmake_minimum_required(VERSION 3.16)
project(voltrum_client)

set(CMAKE_CXX_STANDARD 20)

# Pass linking mode to client
if(VOLTRUM_STATIC_LINKING)
    add_compile_definitions(VOLTRUM_STATIC_LINKING)
else()
    # No API_EXPORT for client - it imports from DLL
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

file(GLOB_RECURSE CLIENT_SRC "src/*.cpp")

add_executable(${PROJECT_NAME} ${CLIENT_SRC})

link_directories(${CMAKE_SOURCE_DIR}/core)

target_link_libraries(${PROJECT_NAME} PRIVATE voltrum_core)
