#include <service/cbprintf.h>
#include <service/unit_test.h>
#include <string>
#include <service/system.h>

static void check_format(const char *format, auto... args)
{
    auto string_appender = [](char c, void *ctx) {
        static_cast<std::string *>(ctx)->append(1, c);
    };

    std::string actual_direct, actual_captured;
    uint8_t buffer[256];
    char expected[256];

    // create expected string
    int written = snprintf(expected, sizeof(expected), format, args...);
    CHECK_TEXT(written >= 0, "Format via standard printf failed!");
    CHECK_TEXT(static_cast<size_t>(written) < sizeof(expected), "Format test buffer too small!");

    // check direct
    cbprintf(string_appender, &actual_direct, format, args...);
    CHECK_EQUAL(std::string(expected), actual_direct);

    // check via capture
    size_t length = cbprintf_capture(buffer, sizeof(buffer), format, args...);
    CHECK_TEXT(length > 0, "Capture test buffer too small?");

    cbprintf_restore(string_appender, &actual_captured, buffer, length);
    CHECK_EQUAL(std::string(expected), actual_captured);
}

TEST_GROUP(cbprintf) {
};

TEST(cbprintf, format_string)
{
    check_format("hello %s!", "world");
}

TEST(cbprintf, format_int)
{
    check_format("hello %d!", 0);
    check_format("hello %i!", 123);
    check_format("hello %i!", -123);
    check_format("hello %d!", INT_MAX);
    check_format("hello %d!", INT_MIN);
}

TEST(cbprintf, format_char)
{
    check_format("hello %hhd!", static_cast<char>(0));
    check_format("hello %hhi!", static_cast<char>(123));
    check_format("hello %hhi!", static_cast<char>(-123));
    check_format("hello %hhd!", CHAR_MAX);
    check_format("hello %hhd!", CHAR_MIN);
}

TEST(cbprintf, format_short)
{
    check_format("hello %hd!", static_cast<short>(0));
    check_format("hello %hi!", static_cast<short>(123));
    check_format("hello %hi!", static_cast<short>(-123));
    check_format("hello %hd!", SHRT_MAX);
    check_format("hello %hd!", SHRT_MIN);
}

TEST(cbprintf, format_long)
{
    check_format("hello %ld!", 0L);
    check_format("hello %li!", 123L);
    check_format("hello %li!", -123L);
    check_format("hello %ld!", LONG_MAX);
    check_format("hello %ld!", LONG_MIN);
}

TEST(cbprintf, format_long_long)
{
    check_format("hello %lld!", 0LL);
    check_format("hello %lli!", 123LL);
    check_format("hello %lli!", -123LL);
    check_format("hello %lld!", LONG_LONG_MAX);
    check_format("hello %lld!", LONG_LONG_MIN);
}

TEST(cbprintf, format_unsigned_int)
{
    check_format("hello %u!", 0U);
    check_format("hello %u!", 123U);
    check_format("hello %u!", UINT_MAX);
}

TEST(cbprintf, format_unsigned_char)
{
    check_format("hello %hhu!", static_cast<unsigned char>(0));
    check_format("hello %hhu!", static_cast<unsigned char>(123));
    check_format("hello %hhu!", UCHAR_MAX);
}

TEST(cbprintf, format_unsigned_short)
{
    check_format("hello %hu!", static_cast<unsigned short>(0));
    check_format("hello %hu!", static_cast<unsigned short>(123));
    check_format("hello %hu!", USHRT_MAX);
}

TEST(cbprintf, format_unsigned_long)
{
    check_format("hello %lu!", 0UL);
    check_format("hello %lu!", 123UL);
    check_format("hello %lu!", ULONG_MAX);
}

TEST(cbprintf, format_unsigned_long_long)
{
    check_format("hello %llu!", 0ULL);
    check_format("hello %llu!", 123ULL);
    check_format("hello %llu!", ULONG_LONG_MAX);
}

TEST(cbprintf, format_size_t)
{
    check_format("hello %zu!", static_cast<size_t>(0));
    check_format("hello %zu!", static_cast<size_t>(123));
    check_format("hello %zd!", SIZE_MAX);
}

TEST(cbprintf, format_hex)
{
    check_format("hello %x!", 0U);
    check_format("hello %hhx!", static_cast<unsigned char>(123));
    check_format("hello %llx!", ULONG_LONG_MAX);
    check_format("hello %llx!", 0x1234567890ULL);
    check_format("hello %llx!", 0xABCDEFULL);
}

TEST(cbprintf, format_ptr)
{
    check_format("hello %p!", reinterpret_cast<void *>(0x1234ABCDU));
    check_format("hello %p!", reinterpret_cast<void *>(0x0U));
    check_format("hello %p!", reinterpret_cast<void *>(0x1U));
}

TEST(cbprintf, format_width)
{
    check_format("hello %5d!", 123);
    check_format("hello %3u!", 1234U);
    check_format("hello %6x!", 0x1234U);
    check_format("hello %2x!", 0x1234U);
}

TEST(cbprintf, format_width_negative)
{
    check_format("hello %3i!", -1234);
    check_format("hello %4i!", -1234);
    check_format("hello %5i!", -1234);
    check_format("hello %6i!", -1234);
}

TEST(cbprintf, format_zero_padding)
{
    check_format("hello %09d!", 123);
    check_format("hello %03u!", 1234U);
    check_format("hello %010x!", 0x1234U);
    check_format("hello %02x!", 0x1234U);
}

TEST(cbprintf, format_zero_padding_negative)
{
    check_format("hello %03i!", -1234);
    check_format("hello %04i!", -1234);
    check_format("hello %05i!", -1234);
    check_format("hello %06i!", -1234);
}

TEST(cbprintf, format_percent)
{
    check_format("hello %d%%!", 55);
}

TEST(cbprintf, format_mixed_arguments)
{
    check_format(
        "hello %d %03u %s %llx %p %i!",
        -123, 44, "test", 0x1234ULL, reinterpret_cast<void *>(0xFFFF), 55
    );
}

TEST(cbprintf, capture_buffer_too_small)
{
    uint8_t buffer[4];
    size_t length = cbprintf_capture(buffer, sizeof(buffer), "%d %d %d", 1, 2, 3);
    CHECK_EQUAL(0, length);
}

TEST(cbprintf, restore_missing_data)
{
    uint8_t buffer[64];
    size_t length = cbprintf_capture(buffer, sizeof(buffer), "%d %d %d", 1, 2, 3);

    length--;

    mock().expectOneCall("system_fatal_error");

    auto null_output = [](char, void *) {
    };
    cbprintf_restore(null_output, nullptr, buffer, length);

    mock().checkExpectations();
    mock().clear();
}

TEST(cbprintf, unknown_specifier_direct)
{
    auto string_appender = [](char c, void *str) {
        static_cast<std::string *>(str)->append(1, c);
    };

    std::string actual;
    cbprintf(string_appender, &actual, "hello %f!", 3.2f);

    // output aborts upon reaching the format specifier
    CHECK_EQUAL(std::string("hello "), actual);
}

TEST(cbprintf, unknown_specifier_capture)
{
    uint8_t buffer[256];
    size_t length = cbprintf_capture(buffer, sizeof(buffer), "hello %f!", 3.2f);
    CHECK_EQUAL(0, length);
}
