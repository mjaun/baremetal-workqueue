#pragma once

#ifdef __cplusplus
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#else
#include <CppUTest/TestHarness_c.h>
#include <CppUTestExt/MockSupport_c.h>

// defined in unit test main.cpp
void ut_print_c_location(const char* text, const char* file, size_t line);

#define UT_PRINT_C(text)  ut_print_c_location(text, __FILE__, __LINE__)

#endif
