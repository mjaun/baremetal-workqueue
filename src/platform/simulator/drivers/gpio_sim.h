#pragma once

#include "drivers/gpio.h"
#include "util/types.h"

struct gpio_pin {
    const char* name;
    bool_t state;
};
