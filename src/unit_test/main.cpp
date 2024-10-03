#include <CppUTest/CommandLineTestRunner.h>

int main(int argc, char* argv[])
{
    return RUN_ALL_TESTS(argc, argv);
}

extern "C" void ut_print_c_location(const char* text, const char* file, size_t line)
{
    UT_PRINT_LOCATION(text, file, line);
}
