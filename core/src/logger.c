#include "logger.h"

#include <stdio.h>   // printf
#include <stdlib.h>  // malloc and free
#include <stdarg.h>  // variadic argument parsing

void _log_output(log_level level, const char* msg, ...) {
    const char* prefix[5] = {"[FATAL]", "[ERROR]", "[WARN]", "[INFO]", "[DEBUG]"};
    char *buffer;
    int32_t len;

    __builtin_va_list va_list_ptr;
    va_start(va_list_ptr, msg);

    // Call vsnprintf twice, first with a copy of the vararg list/pointer
    // to find the total length once formatted (throwing result away).
    va_list va_list_ptr_copy;
    va_copy(va_list_ptr_copy, va_list_ptr);
    len = vsnprintf(0, 0, msg, va_list_ptr_copy);
    va_end(va_list_ptr_copy);

    // Allocate known length from above vsnprintf and call it again, this
    // time writing the result to the buffer.
    buffer = malloc(len + 1);
    vsnprintf(buffer, len + 1, msg, va_list_ptr);
    buffer[len] = '\0';
    va_end(va_list_ptr);

    printf("%s: %s\n", prefix[level], buffer);
    free(buffer);
}
