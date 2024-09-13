#pragma once

typedef struct gpio_pin* gpio_pin_handle_t;
typedef struct uart* uart_handle_t;

struct peripherals {
    gpio_pin_handle_t user_led;
    uart_handle_t debug_uart;
};

extern const struct peripherals peripherals;
