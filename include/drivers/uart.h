#pragma once

#include "util/types.h"

struct uart;

void uart_write(struct uart* uart, const uint8_t* data, size_t length);
