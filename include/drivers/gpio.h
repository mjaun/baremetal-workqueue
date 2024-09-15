#pragma once

struct gpio_pin;

/**
 * Toggles the output state of the GPIO pin.
 *
 * @param pin GPIO pin to toggle.
 */
void gpio_toggle(struct gpio_pin *pin);
