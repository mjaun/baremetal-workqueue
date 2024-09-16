#pragma once

#include "util/types.h"

struct uart;

/**
 * Writes data via UART.
 *
 * @param uart UART interface to write to.
 * @param data Data to write.
 * @param length Length of data in bytes.
 */
void uart_write(struct uart* uart, const uint8_t* data, size_t length);
