#pragma once

struct gpio_pin;
struct uart;

/**
 * Struct containing handles for all peripherals accessed by the application.
 */
struct peripherals {
    struct gpio_pin* user_led;
    struct uart* debug_uart;
};

/**
 * Struct containing handles for all peripherals accessed by the application.
 */
extern const struct peripherals peripherals;
