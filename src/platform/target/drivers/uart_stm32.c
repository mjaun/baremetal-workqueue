#include "uart_stm32.h"

void uart_write(struct uart *uart, const uint8_t *data, size_t length)
{
    HAL_UART_Transmit(uart->huart, data, length, HAL_MAX_DELAY);
}
