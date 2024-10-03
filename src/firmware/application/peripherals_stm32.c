#include <application/peripherals.h>
#include <driver/uart_stm32.h>
#include <driver/gpio_stm32.h>
#include <main.h>

extern UART_HandleTypeDef huart2;

static struct gpio_pin gpio_pin_user_led = {
    .port = LD2_GPIO_Port,
    .pin = LD2_Pin,
};

static struct gpio_pin gpio_pin_user_button = {
    .port = B1_GPIO_Port,
    .pin = B1_Pin,
};

static struct uart uart_debug = {
    .huart = &huart2,
};

const struct peripherals peripherals = {
    .user_led = &gpio_pin_user_led,
    .user_button = &gpio_pin_user_button,
    .debug_uart = &uart_debug,
};
