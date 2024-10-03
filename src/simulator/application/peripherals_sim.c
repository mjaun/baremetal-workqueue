#include <application/peripherals_sim.h>
#include <driver/uart_sim.h>
#include <driver/gpio_sim.h>

static struct gpio_pin gpio_user_led;
static struct gpio_pin gpio_user_button;
static struct uart uart_debug;

const struct peripherals peripherals = {
    .user_led = &gpio_user_led,
    .user_button = &gpio_user_button,
    .debug_uart = &uart_debug,
};

void peripherals_setup(void)
{
    gpio_pin_init(&gpio_user_led, "user_led", false);
    gpio_pin_init(&gpio_user_button, "user_button", false);
}
