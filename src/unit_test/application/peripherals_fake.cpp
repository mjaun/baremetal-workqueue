#include "application/peripherals.h"

struct gpio_pin
{
    uint8_t dummy;
};

struct uart
{
    uint8_t dummy;
};

static struct gpio_pin gpio_user_led;
static struct gpio_pin gpio_user_button;

static struct uart uart_debug;

extern "C" const struct peripherals peripherals = {
    .user_led = &gpio_user_led,
    .user_button = &gpio_user_button,
    .debug_uart = &uart_debug,
};
