#pragma once

#include <driver/uart.h>
#include <stm32f4xx_hal.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uart {
    UART_HandleTypeDef* huart;
};

#ifdef __cplusplus
}
#endif