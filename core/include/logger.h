#pragma once

#include "defines.h"

typedef enum {
    // Application should stop
    LOG_LEVEL_FATAL = 0,
    // Critial incorrect behaviour
    LOG_LEVEL_ERROR = 1,
    // Non-critical incorrect behaviour
    LOG_LEVEL_WARN = 2,
    // No error, informative
    LOG_LEVEL_INFO = 3,
    // Debug use
    LOG_LEVEL_DEBUG = 4,
} log_level;

// To be used indirectly with the LOG_[LEVEL] macros (e.g. LOG_ERROR).
CORE_API void _log_output(log_level level, const char* msg, ...);

#define LOG_FATAL(msg, ...) _log_output(LOG_LEVEL_FATAL, msg, ##__VA_ARGS__);
#define LOG_ERROR(msg, ...) _log_output(LOG_LEVEL_ERROR, msg, ##__VA_ARGS__);
#define LOG_WARN(msg, ...)  _log_output(LOG_LEVEL_WARN,  msg, ##__VA_ARGS__);
#define LOG_INFO(msg, ...)  _log_output(LOG_LEVEL_INFO,  msg, ##__VA_ARGS__);
#define LOG_DEBUG(msg, ...) _log_output(LOG_LEVEL_DEBUG, msg, ##__VA_ARGS__);
