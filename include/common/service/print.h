#pragma once

#include "util/types.h"
#include <stdarg.h>

typedef void (*print_func_t)(char c);

void print_format(print_func_t out, const char *format, ...);
size_t print_capture(void *packaged, size_t length, const char *format, va_list ap);
void print_output(print_func_t out, const void *packaged, size_t length);
