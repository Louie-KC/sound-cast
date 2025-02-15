#include "assert.h"
#include "logger.h"
#include <stdio.h>

void _assert_report_fail(const char* expr, const char *msg, const char* file_name, int32_t line) {
    char buffer[200];
    if (!msg) {
        sprintf(buffer, "Assertion fail: %s, %s:%d", expr, file_name, line);
    } else {
        sprintf(buffer, "Assertion fail: %s, '%s', %s:%d", expr, msg, file_name, line);
    }
    LOG_FATAL(buffer);
}
