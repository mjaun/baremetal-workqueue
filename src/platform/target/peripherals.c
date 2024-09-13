#include "application/peripherals.h"
#include "drivers/uart_stm32.h"
#include "drivers/gpio_stm32.h"
#include "main.h"

extern UART_HandleTypeDef huart2;

static const struct gpio_pin gpio_pin_user_led = {
    .port = LD2_GPIO_Port,
    .pin = LD2_Pin,
};

static const struct uart uart_debug = {
    .huart = &huart2,
};

const struct peripherals peripherals = {
    .user_led = (gpio_pin_handle_t) &gpio_pin_user_led,
    .debug_uart = (uart_handle_t) &uart_debug,
};
