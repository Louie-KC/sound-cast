#pragma once

#include <stdint.h>

// True and false
#define true 1
#define false 0

// Expose functions
#ifdef API_EXPORT
#define CORE_API __attribute__((visibility("default")))
#else
#define CORE_API
#endif
