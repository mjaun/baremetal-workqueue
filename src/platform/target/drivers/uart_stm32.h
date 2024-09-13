#pragma once

#include "drivers/uart.h"
#include <stm32f4xx_hal.h>

struct uart {
    UART_HandleTypeDef* huart;
};
