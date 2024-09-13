#pragma once

#include "drivers/gpio.h"
#include <stm32f4xx_hal.h>

struct gpio_pin {
    GPIO_TypeDef* port;
    uint16_t pin;
};
