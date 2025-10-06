#include "log.h"
#include <stdio.h>
#include <stdarg.h>

void game_log(LogLevel level, const char *module, const char *fmt, ...) {

    #ifdef PLATFORM_PC
    // Only log on PC builds
    const char *level_str;
    switch (level) {
        case LOG_ERROR: level_str = "ERROR"; break;
        case LOG_WARN:  level_str = "WARN "; break;
        case LOG_INFO:  level_str = "INFO "; break;
        case LOG_DEBUG: level_str = "DEBUG"; break;
        default:        level_str = "?????"; break;

}

    fprintf(stderr, "[%s][%s] ", level_str, module ? module : "???");

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
#else
    // No-op on Vita
    (void)level;
    (void)module;
    (void)fmt;
#endif
}
