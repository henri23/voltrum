#pragma once

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using s8 = signed char;
using s16 = signed short;
using s32 = signed int;
using s64 = signed long long;

using f32 = float;
using f64 = double;

using b32 = int;
using b8 = bool;

constexpr u32 INVALID_ID = -1;
using Object_ID = u32;

// Properly define static assertions
#if defined(__clang__) || defined(__gcc__)
#    define STATIC_ASSERT static_assert
#else
#    define STATIC_ASSERT static_assert
#endif

STATIC_ASSERT(sizeof(u8) == 1, "Expected u8 to be 1 byte");
STATIC_ASSERT(sizeof(u16) == 2, "Expected u16 to be 2 bytes");
STATIC_ASSERT(sizeof(u32) == 4, "Expected u32 to be 4 bytes");
STATIC_ASSERT(sizeof(u64) == 8, "Expected u64 to be 8 bytes");

STATIC_ASSERT(sizeof(s8) == 1, "Expected s8 to be 1 byte");
STATIC_ASSERT(sizeof(s16) == 2, "Expected s16 to be 2 bytes");
STATIC_ASSERT(sizeof(s32) == 4, "Expected s32 to be 4 bytes");
STATIC_ASSERT(sizeof(s64) == 8, "Expected s64 to be 8 bytes");

STATIC_ASSERT(sizeof(f32) == 4, "Expected f32 to be 4 bytes");
STATIC_ASSERT(sizeof(f64) == 8, "Expected f64 to be 8 bytes");

constexpr u64 GIB(1 << 30);
constexpr u64 MIB(1 << 20);
constexpr u64 KIB(1 << 10);

#define local_persist static
#define internal_variable static
#define global_variable static

#define INTERNAL_FUNC static

// Platform detection
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#    define PLATFORM_WINDOWS 1
#    ifndef _WIN64
#        error "64-bit is required on Windows!"
#    endif
#elif defined(__linux__) || defined(__gnu_linux__)
// Linux OS
#    define PLATFORM_LINUX 1
#    if defined(__ANDROID__)
#        define PLATFORM_ANDROID 1
#    endif
#elif defined(__unix__)
// Catch anything not caught by the above.
#    define PLATFORM_UNIX 1
#elif defined(_POSIX_VERSION)
// Posix
#    define PLATFORM_POSIX 1
#elif __APPLE__
// Apple platforms
#    define PLATFORM_APPLE 1
#    include <TargetConditionals.h>
#    if TARGET_IPHONE_SIMULATOR
// iOS Simulator
#        define PLATFORM_IOS 1
#        define PLATFORM_IOS_SIMULATOR 1
#    elif TARGET_OS_IPHONE
#        define PLATFORM_IOS 1
// iOS device
#    elif TARGET_OS_MAC
// Other kinds of macOS
#    else
#        error "Unknown Apple platform"
#    endif
#else
#    error "Unknown platform!"
#endif

/*
  Flexible linking support - static or dynamic
  Controlled by VOLTRUM_STATIC_LINKING preprocessor definition
*/
#ifdef VOLTRUM_STATIC_LINKING
// Static linking - no export/import needed
#    define VOLTRUM_API
#else
// Dynamic linking - use DLL export/import
#    ifdef API_EXPORT
// Exports when building the core DLL
#        ifdef _MSC_VER
#            define VOLTRUM_API __declspec(dllexport)
#        else
#            define VOLTRUM_API __attribute__((visibility("default")))
#        endif
#    else
// Imports when using the core DLL from client
#        ifdef _MSC_VER
#            define VOLTRUM_API __declspec(dllimport)
#        else
#            define VOLTRUM_API
#        endif
#    endif
#endif

#define CLAMP(value, min, max)                                                 \
    ((value > max) ? max : (value < min) ? min : value)

// Inlining - An inline function is substituted at compile time, by the compiler
// to the location is has been called, but rather than storing the function in
// memory and copying the result values into the destination, it completelly
// copies the logic into that destination, so there is no overhead from function
// calls or result copying. Usually the compiler tries to always achieve this
// but by using the forcing keywords, we make the compiler "try harder"
#ifdef _MSC_VER
#    define FORCE_INLINE __forceinline
#    define FORCE_NOT_INLINE __declspec(noinline)
#else
#    define FORCE_INLINE inline
#    define FORCE_NOT_INLINE
#endif

// For operator overloading in headers - must use 'inline' (not __forceinline)
// to satisfy One Definition Rule (ODR) across all compilers
#define INLINE_OPERATOR inline
