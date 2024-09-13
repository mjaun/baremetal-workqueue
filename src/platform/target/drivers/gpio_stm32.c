#include "gpio_stm32.h"

void gpio_toggle(struct gpio_pin *pin)
{
    HAL_GPIO_TogglePin(pin->port, pin->pin);
}
