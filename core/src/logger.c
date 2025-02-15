#include "logger.h"

#include <stdio.h>

void _log_output(log_level level, const char* msg) {
    const char* prefix[5] = {"[FATAL]", "[ERROR]", "[WARN]", "[INFO]", "[DEBUG]"};
    char buffer[LOG_MAX_LEN];

    sprintf(buffer, "%s: %s\n", prefix[level], msg);
    buffer[LOG_MAX_LEN - 1] = '\0';

    printf("%s", buffer);
}
