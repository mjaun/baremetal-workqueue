#include "driver/uart_sim.h"
#include "util/unused.h"
#include <stdio.h>

void uart_write(struct uart *uart, const uint8_t *data, size_t length)
{
    ARG_UNUSED(uart);

    for (size_t i = 0; i < length; i++) {
        putc(data[i], stdout);
    }
}
