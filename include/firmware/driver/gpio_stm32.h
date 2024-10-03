#pragma once

#include <driver/gpio.h>
#include <stm32f4xx_hal.h>

#ifdef __cplusplus
extern "C" {
#endif

struct gpio_pin {
    GPIO_TypeDef *port;
    uint16_t pin;

    exti_handler_t exti_handler;
    struct gpio_pin *exti_next;
};

#ifdef __cplusplus
}
#endif
