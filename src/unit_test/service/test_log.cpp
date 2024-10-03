#include <service/log.h>
#include <service/work.h>
#include <service/system.h>
#include <service/unit_test.h>
#include <string>
#include <vector>
#include <regex>
#include <sstream>
#include <iomanip>

#define ANY_TIMESTAMP "\\[[0-9]{2,}:[0-9]{2}:[0-9]{2}.[0-9]{3},[0-9]{3}\\] "

LOG_MODULE_REGISTER(test_log);

static bool new_line = true;
static std::vector<std::string> output_lines;

static void match_line(ssize_t index, const std::string &regex)
{
    if (index < 0) {
        index = static_cast<ssize_t>(output_lines.size()) + index;
    }

    // get line replacing any ANSI escape codes
    std::regex ansi_escape("\\x1B\\[[0-?]*[ -/]*[@-~]");
    std::string line = std::regex_replace(output_lines[index], ansi_escape, "");

    // match line against given regex
    std::string message = "line='" + line + "' regex='" + regex + "'";
    CHECK_TRUE_TEXT(std::regex_match(line, std::regex(regex)), message.c_str());
}

static std::string format_timestamp(u64_us_t uptime)
{
    uint64_t total_seconds = uptime / 1'000'000;
    uint64_t remaining_microseconds = uptime % 1'000'000;

    uint64_t hours = total_seconds / 3600;
    uint64_t minutes = (total_seconds % 3600) / 60;
    uint64_t seconds = total_seconds % 60;
    uint64_t milliseconds = remaining_microseconds / 1'000;
    uint64_t microseconds_only = remaining_microseconds % 1'000;

    std::stringstream stream;

    stream << "\\[";
    stream << std::setfill('0') << std::setw(2) << hours;
    stream << ":";
    stream << std::setw(2) << minutes;
    stream << ":";
    stream << std::setw(2) << seconds;
    stream << "\\.";
    stream << std::setw(3) << milliseconds;
    stream << ",";
    stream << std::setw(3) << microseconds_only;
    stream << "\\] ";

    return stream.str();
}

// override to capture output
void system_debug_out(char c)
{
    if (new_line) {
        if (c != '\n') {
            output_lines.emplace_back(1, c);
            new_line = false;
        }
    } else {
        if (c != '\n') {
            output_lines.back().append(1, c);
        } else {
            UT_PRINT_LOCATION(output_lines.back().c_str(), "OUT", output_lines.size());
            new_line = true;
        }
    }
}

TEST_GROUP(log) {
    void setup() override
    {
        output_lines.clear();
        new_line = true;

        // make sure uptime is not zero and different uptimes are used for each test
        system_busy_sleep_us(123'456);
    }
};

TEST(log, level_all)
{
    log_set_level("test_log", LOG_LEVEL_DBG);

    LOG_DBG("debug");
    LOG_INF("information");
    LOG_WRN("warning");
    LOG_ERR("error");

    work_run_for(0);

    CHECK_EQUAL(4, output_lines.size());
    match_line(0, ANY_TIMESTAMP "<dbg> test_log: debug");
    match_line(1, ANY_TIMESTAMP "<inf> test_log: information");
    match_line(2, ANY_TIMESTAMP "<wrn> test_log: warning");
    match_line(3, ANY_TIMESTAMP "<err> test_log: error");
}

TEST(log, level_filtered)
{
    log_set_level("test_log", LOG_LEVEL_WRN);

    LOG_DBG("debug");
    LOG_INF("information");
    LOG_WRN("warning");
    LOG_ERR("error");

    CHECK_EQUAL(0, output_lines.size());
    work_run_for(0);

    CHECK_EQUAL(2, output_lines.size());
    match_line(0, ANY_TIMESTAMP "<wrn> test_log: warning");
    match_line(1, ANY_TIMESTAMP "<err> test_log: error");
}

TEST(log, buffer_overflow)
{
    // log a message
    LOG_INF("hello");

    // nothing printed yet
    CHECK_EQUAL(0, output_lines.size());

    work_run_for(0);

    // message is now printed
    CHECK_EQUAL(1, output_lines.size());
    match_line(0, ANY_TIMESTAMP "<inf> test_log: hello");

    // print many messages
    for (size_t i = 0; i < 10000; i++) {
        LOG_INF("spam");
    }

    work_run_for(0);

    // dropped message is logged plus any message which did fit into the buffer
    CHECK_TRUE(3 < output_lines.size());
    match_line(1, "--- [0-9]+ messages dropped ---");
    match_line(2, ANY_TIMESTAMP "<inf> test_log: spam");
    match_line(3, ANY_TIMESTAMP "<inf> test_log: spam");

    // logging can continue normally
    LOG_INF("world");
    work_run_for(0);
    match_line(-1, ANY_TIMESTAMP "<inf> test_log: world");
}

TEST(log, format_string)
{
    LOG_INF("hello %s!", "world");
    LOG_INF("hello %d!", 42);
    LOG_INF("hello %06u %06x!", 123U, 0x456U);

    work_run_for(0);

    CHECK_EQUAL(3, output_lines.size());
    match_line(0, ANY_TIMESTAMP "<inf> test_log: hello world!");
    match_line(1, ANY_TIMESTAMP "<inf> test_log: hello 42!");
    match_line(2, ANY_TIMESTAMP "<inf> test_log: hello 000123 000456!");
}

TEST(log, timestamp)
{
    // log messages
    u64_us_t begin = system_uptime_get_us();
    LOG_INF("begin");

    system_busy_sleep_us(456);
    u64_us_t after_us = system_uptime_get_us();
    LOG_INF("after microseconds");

    system_busy_sleep_ms(789);
    u64_us_t after_ms = system_uptime_get_us();
    LOG_INF("after milliseconds");

    system_busy_sleep_ms(23 * 1000);
    u64_us_t after_s = system_uptime_get_us();
    LOG_INF("after seconds");

    system_busy_sleep_ms(34 * 1000 * 60);
    u64_us_t after_min = system_uptime_get_us();
    LOG_INF("after minutes");

    system_busy_sleep_ms(13 * 1000 * 60 * 60);
    u64_us_t after_h = system_uptime_get_us();
    LOG_INF("after hours");

    system_busy_sleep_ms(20ULL * 1000 * 60 * 60 * 24 * 365);
    u64_us_t after_y = system_uptime_get_us();
    LOG_INF("after years");

    // print messages
    CHECK_EQUAL(0, output_lines.size());
    work_run_for(0);
    CHECK_EQUAL(7, output_lines.size());

    match_line(0, format_timestamp(begin) + "<inf> test_log: begin");
    match_line(1, format_timestamp(after_us) + "<inf> test_log: after microseconds");
    match_line(2, format_timestamp(after_ms) + "<inf> test_log: after milliseconds");
    match_line(3, format_timestamp(after_s) + "<inf> test_log: after seconds");
    match_line(4, format_timestamp(after_min) + "<inf> test_log: after minutes");
    match_line(5, format_timestamp(after_h) + "<inf> test_log: after hours");
    match_line(6, format_timestamp(after_y) + "<inf> test_log: after years");
}
