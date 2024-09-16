#include "application/peripherals.h"
#include "application/uptime_printer.h"
#include "service/work_ext.h"
#include "service/system.h"
#include "service/unit_test.h"
#include <string>
#include <sstream>
#include <iomanip>

static std::string get_uptime_message(u64_us_t timestamp);

TEST_GROUP(uptime_printer)
{
};

TEST(uptime_printer, print_after_init)
{
    // Arrange
    u64_us_t uptime = system_uptime_get();
    std::string expected_message = get_uptime_message(uptime);

    mock()
            .expectOneCall("uart_write")
            .withPointerParameter("uart", peripherals.debug_uart)
            .withParameter("data", reinterpret_cast<const uint8_t *>(expected_message.data()),
                           expected_message.length())
            .withParameter("length", expected_message.length());

    // Act
    uptime_printer_init();
    work_run_for(0);

    // Assert
    mock().checkExpectations();
    mock().clear();
}

TEST(uptime_printer, print_periodically)
{
    // Arrange
    mock()
            .expectNCalls(10, "uart_write")
            .ignoreOtherParameters();

    // Act
    uptime_printer_init();

    work_run_for(9'500);

    // Assert
    mock().checkExpectations();
    mock().clear();
}

static std::string get_uptime_message(u64_us_t timestamp)
{
    std::ostringstream stream;
    stream << "up-time: ";
    stream << std::setw(6) << (timestamp / 1'000'000ULL);
    stream << ".";
    stream << std::setw(3) << std::setfill('0') << (timestamp / 1'000ULL % 1'000ULL);
    stream << ",";
    stream << std::setw(3) << std::setfill('0') << (timestamp % 1'000ULL);
    stream << "\r\n";
    return stream.str();
}
