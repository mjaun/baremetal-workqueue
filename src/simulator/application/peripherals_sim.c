#include "application/peripherals.h"
#include "driver/uart_sim.h"
#include "driver/gpio_sim.h"

static struct gpio_pin gpio_user_led = {
    .name = "user_led",
};

static struct gpio_pin gpio_user_button = {
    .name = "user_button",
};

static struct uart uart_debug = {0};

const struct peripherals peripherals = {
    .user_led = &gpio_user_led,
    .user_button = &gpio_user_button,
    .debug_uart = &uart_debug,
};
