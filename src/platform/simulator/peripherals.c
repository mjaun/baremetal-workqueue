#include "application/peripherals.h"
#include "drivers/uart_sim.h"
#include "drivers/gpio_sim.h"

static struct gpio_pin gpio_user_led = {
    .name = "LD2",
    .state = false,
};

static struct uart uart_debug = {0};

const struct peripherals peripherals = {
    .user_led = &gpio_user_led,
    .debug_uart = &uart_debug,
};
