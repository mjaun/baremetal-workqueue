#pragma once

#include "util/types.h"

typedef void (*print_func_t)(char c);

size_t print_package(void *packaged, size_t length, const char *format, ...);
void print_output(print_func_t out, const void *packaged, size_t length);
