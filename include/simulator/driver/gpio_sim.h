#pragma once

#include <driver/gpio.h>
#include <service/adapter_sim.h>
#include <util/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct gpio_pin {
    const char* name;
    bool_t state;
    exti_handler_t callback;
    struct adapter adapter;
};

void gpio_pin_init(struct gpio_pin* pin, const char* name, bool_t initial_state);

#ifdef __cplusplus
}
#endif
