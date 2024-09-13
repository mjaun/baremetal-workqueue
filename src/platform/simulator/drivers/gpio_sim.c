#include "gpio_sim.h"
#include <stdio.h>

void gpio_toggle(struct gpio_pin *pin)
{
    pin->state = !pin->state;
    printf("GPIO pin %s: %s\r\n", pin->name, pin->state ? "on" : "off");
}
