#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>

// requires at least C11
#define BUILD_ASSERT(EXPR)      static_assert(EXPR)

#define RUNTIME_ASSERT(EXPR) \
    if (!(EXPR)) { \
        assert_handler(__FILENAME__, __LINE__); \
    } \
    do {} while (0)

void assert_handler(const char* file, unsigned line);

#ifdef __cplusplus
}
#endif
