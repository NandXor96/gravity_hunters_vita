#pragma once

/**
 * @brief Centralized logging for error and debug messages.
 *
 * Logging is only active on PC builds (PLATFORM_PC).
 * On Vita builds, log calls compile to no-ops for performance.
 */

typedef enum {
    LOG_ERROR,   // Critical errors
    LOG_WARN,    // Warnings
    LOG_INFO,    // Informational messages
    LOG_DEBUG    // Debug output
} LogLevel;

/**
 * @brief Log a message with module context.
 *
 * @param level Log level (ERROR, WARN, INFO, DEBUG)
 * @param module Module/component name (e.g., "app", "world", "renderer")
 * @param fmt Printf-style format string
 * @param ... Format arguments
 */
void game_log(LogLevel level, const char *module, const char *fmt, ...);

/**
 * @brief Convenience macros for logging.
 */
#define LOG_ERROR(module, ...) game_log(LOG_ERROR, module, __VA_ARGS__)
#define LOG_WARN(module, ...)  game_log(LOG_WARN, module, __VA_ARGS__)
#define LOG_INFO(module, ...)  game_log(LOG_INFO, module, __VA_ARGS__)
#define LOG_DEBUG(module, ...) game_log(LOG_DEBUG, module, __VA_ARGS__)
