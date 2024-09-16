#include "driver/gpio_sim.h"
#include "util/unused.h"
#include <stdio.h>

void gpio_toggle(struct gpio_pin *pin)
{
    pin->state = !pin->state;
    printf("GPIO pin %s: %s\r\n", pin->name, pin->state ? "on" : "off");
}

void gpio_exti_callback(struct gpio_pin *pin, exti_handler_t handler)
{
    ARG_UNUSED(pin);
    ARG_UNUSED(handler);

    // not supported yet
}
