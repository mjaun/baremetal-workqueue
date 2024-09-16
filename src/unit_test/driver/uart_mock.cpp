#include "service/unit_test.h"
#include "driver/uart.h"

extern "C" void uart_write(struct uart* uart, const uint8_t* data, size_t length)
{
    mock().actualCall(__func__)
        .withPointerParameter("uart", uart)
        .withParameter("data", data, length)
        .withParameter("length", length);
}
