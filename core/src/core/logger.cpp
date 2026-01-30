#include "logger.hpp"

#include "spdlog/logger.h"
#include "spdlog/spdlog.h"

#include <filesystem> // TODO: Create custom filesystem library or find one
#include <stdarg.h>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

// Consistent log format for all loggers
internal_var const char *LOG_PATTERN =
    "%^[%Y-%m-%d %H:%M:%S.%e] [%-12n] [%-7l] %v%$";

// Smart pointer loggers
internal_var std::shared_ptr<spdlog::logger> core_logger = nullptr;
internal_var std::shared_ptr<spdlog::logger> client_logger = nullptr;

// Helper function to create logger with consistent format and file output
INTERNAL_FUNC std::shared_ptr<spdlog::logger>
create_logger(const char *logger_name, const char *file_name) {

    // TODO: Change later and move away from <filesystem> library
    // Ensure logs directory exists
    std::filesystem::create_directories("logs");

    // Create file path
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "logs/%s", file_name);

    // Create sinks: console + daily rotating file
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink =
        std::make_shared<spdlog::sinks::daily_file_sink_mt>(file_path,
            0,
            0,
            true,
            7); // Daily rotation at midnight

    // Set consistent formatting for all sinks
    console_sink->set_pattern(LOG_PATTERN);
    file_sink->set_pattern(LOG_PATTERN);

    // Create logger with both sinks
    spdlog::sink_ptr sinks[] = {console_sink, file_sink};
    auto logger = std::make_shared<spdlog::logger>(logger_name,
        std::begin(sinks),
        std::end(sinks));

    // Set level and register
    logger->set_level(spdlog::level::trace);
    spdlog::register_logger(logger);

    return logger;
}

// Create default console logger with consistent formatting
INTERNAL_FUNC std::shared_ptr<spdlog::logger> _create_default_logger() {

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern(LOG_PATTERN);

    auto logger =
        std::make_shared<spdlog::logger>("default_console", console_sink);

    logger->set_level(spdlog::level::trace);

    return logger;
}

// Default console logger that's always available. Its lifetime is tied to the
// static lifetime instead of being managed by the log sybsystem, so that I
// always have a valid logger before and after the log system has been init/shut
// down
global_variable std::shared_ptr<spdlog::logger> default_console_logger =
    _create_default_logger();

b8 log_init() {
    try {
        // Create core logger with file output
        core_logger = create_logger("voltrum_core", "core.log");

        // Create client logger with file output
        client_logger = create_logger("client_app", "client.log");

        // Set default logger to core
        spdlog::set_default_logger(core_logger);
        spdlog::set_level(spdlog::level::trace);

        CORE_DEBUG("Log subsystem initialized.");

        return true;
    } catch (const std::exception &ex) {
        // Log initialization failure to default logger
        default_console_logger->error("Logger initialization failed: {}",
            ex.what());

        return false;
    }
}

void log_shutdown() {
    CORE_DEBUG("Logger shutting down...");

    // Flush all loggers before shutdown
    if (core_logger)
        core_logger->flush();

    if (client_logger)
        client_logger->flush();

    // Reset smart pointers
    core_logger.reset();
    client_logger.reset();

    // Drop all registered loggers
    spdlog::drop_all();

    // Shutdown spdlog completely
    spdlog::shutdown();

    CORE_DEBUG("Logger shut down.");
}

void log_output(Log_Scope scope, Log_Level level, const char *message, ...) {

    // Select the appropriate logger based on scope
    std::shared_ptr<spdlog::logger> logger = nullptr;

    if (core_logger && client_logger) {
        switch (scope) {
        case Log_Scope::CORE:
            logger = core_logger;
            break;
        case Log_Scope::CLIENT:
            logger = client_logger;
            break;
        default:
            logger = default_console_logger;
            break;
        }
    }

    // Fall back to default console logger if main loggers aren't available
    if (!logger) {
        logger = default_console_logger;
    }

    // Handle variable arguments
    va_list args;
    va_start(args, message);
    char formatted_message[4096];
    vsnprintf(formatted_message, sizeof(formatted_message), message, args);
    va_end(args);

    // Log using spdlog
    switch (level) {
    case Log_Level::FATAL:
        logger->critical(formatted_message);
        break;
    case Log_Level::ERROR:
        logger->error(formatted_message);
        break;
    case Log_Level::WARN:
        logger->warn(formatted_message);
        break;
    case Log_Level::INFO:
        logger->info(formatted_message);
        break;
    case Log_Level::DEBUG:
        logger->debug(formatted_message);
        break;
    case Log_Level::TRACE:
        logger->trace(formatted_message);
        break;
    default:
        logger->info(formatted_message);
        break;
    }
}

VOLTRUM_API void report_assertion_failure(const char *expression,
    const char *message,
    const char *file,
    s32 line) {

    default_console_logger->critical(
        "Assertion failure: {} failed with message '{}', file {}, line {}",
        expression,
        message,
        file,
        line);
}
