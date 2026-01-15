#pragma once

#include "defines.hpp"

#define LOG_WARN_ENABLED 1
#define LOG_INFO_ENABLED 1
#define LOG_DEBUG_ENABLED 1
#define LOG_TRACE_ENABLED 1

#if RELEASE_BUILD == 1
#    define LOG_DEBUG_ENABLED 0
#    define LOG_TRACE_ENABLED 0
#endif

enum class Log_Level : u8 { FATAL = 0, ERROR, WARN, INFO, DEBUG, TRACE };

enum class Log_Scope : u8 { CORE = 0, CLIENT };

b8 log_init();
void log_shutdown();

VOLTRUM_API void
log_output(Log_Scope scope, Log_Level level, const char *message, ...);

// The __VA_ARGS__ is the way clang/gcc handles variable arguments
#define CORE_FATAL(message, ...)                                               \
    log_output(Log_Scope::CORE, Log_Level::FATAL, message, ##__VA_ARGS__);
#define CLIENT_FATAL(message, ...)                                             \
    log_output(Log_Scope::CLIENT, Log_Level::FATAL, message, ##__VA_ARGS__);

#ifndef CORE_ERROR
#    define CORE_ERROR(message, ...)                                           \
        log_output(Log_Scope::CORE, Log_Level::ERROR, message, ##__VA_ARGS__);
#endif

#ifndef CLIENT_ERROR
#    define CLIENT_ERROR(message, ...)                                         \
        log_output(Log_Scope::CLIENT, Log_Level::ERROR, message, ##__VA_ARGS__);
#endif

#if LOG_WARN_ENABLED == 1
#    define CORE_WARN(message, ...)                                            \
        log_output(Log_Scope::CORE, Log_Level::WARN, message, ##__VA_ARGS__);
#    define CLIENT_WARN(message, ...)                                          \
        log_output(Log_Scope::CLIENT, Log_Level::WARN, message, ##__VA_ARGS__);
#else
#    define CORE_WARN(message, ...)
#    define CLIENT_WARN(message, ...)
#endif

#if LOG_TRACE_ENABLED == 1
#    define CORE_TRACE(message, ...)                                           \
        log_output(Log_Scope::CORE, Log_Level::TRACE, message, ##__VA_ARGS__);
#    define CLIENT_TRACE(message, ...)                                         \
        log_output(Log_Scope::CLIENT, Log_Level::TRACE, message, ##__VA_ARGS__);
#else
#    define CORE_TRACE(message, ...)
#    define CLIENT_TRACE(message, ...)
#endif

#if LOG_INFO_ENABLED == 1
#    define CORE_INFO(message, ...)                                            \
        log_output(Log_Scope::CORE, Log_Level::INFO, message, ##__VA_ARGS__);
#    define CLIENT_INFO(message, ...)                                          \
        log_output(Log_Scope::CLIENT, Log_Level::INFO, message, ##__VA_ARGS__);
#else
#    define CORE_INFO(message, ...)
#    define CLIENT_INFO(message, ...)
#endif

#if LOG_DEBUG_ENABLED == 1
#    define CORE_DEBUG(message, ...)                                           \
        log_output(Log_Scope::CORE, Log_Level::DEBUG, message, ##__VA_ARGS__);
#    define CLIENT_DEBUG(message, ...)                                         \
        log_output(Log_Scope::CLIENT, Log_Level::DEBUG, message, ##__VA_ARGS__);
#else
#    define CORE_DEBUG(message, ...)
#    define CLIENT_DEBUG(message, ...)
#endif
