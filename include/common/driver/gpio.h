#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct gpio_pin;

typedef void (*exti_handler_t)(struct gpio_pin *pin);

/**
 * Toggles the output state of the GPIO pin.
 *
 * @param pin GPIO pin to toggle.
 */
void gpio_toggle(struct gpio_pin *pin);

/**
 * Sets the callback function for a GPIO pin with external interrupt.
 *
 * The callback function is invoked in ISR context.
 *
 * @param pin GPIO pin to set the callback for.
 * @param handler Callback function to invoke.
 */
void gpio_exti_callback(struct gpio_pin *pin, exti_handler_t handler);

#ifdef __cplusplus
}
#endif
