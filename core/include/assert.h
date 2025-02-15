#pragma once

#include "defines.h"

#include <stdlib.h>

CORE_API void _assert_report_fail(const char* expr, const char *msg, const char* file_name, int32_t line);

/*
 * An assertion on `expr`. 
 * 
 * If `expr` does not evaluate to true/a truthy value:
 * 1. A fatal error is logged.
 * 2. The process exits.
 */
#define CORE_ASSERT(expr) {\
    if (expr) {\
    } else {\
        _assert_report_fail(#expr, NULL, __FILE__, __LINE__);\
        exit(1);\
    }\
}\

/*
 * An assertion on `expr`. 
 * 
 * If `expr` does not evaluate to true/a truthy value:
 * 1. A fatal error is logged with the provided `msg`.
 * 2. The process exits.
 */
#define CORE_ASSERT_MSG(expr, msg) {\
    if (expr) {\
    } else {\
        _assert_report_fail(#expr, msg, __FILE__, __LINE__);\
        exit(1);\
    }\
}\
