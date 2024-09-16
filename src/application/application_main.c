#include "application/peripherals.h"
#include "services/work.h"
#include "drivers/gpio.h"
#include "drivers/uart.h"
#include "drivers/system.h"
#include "util/unused.h"
#include <string.h>
#include <stdio.h>

static void exti_handler(struct gpio_pin *pin);

static void print_uptime(struct work *work);
static void print_button_pressed(struct work *work);

static WORK_DEFINE(work_uptime, 1, print_uptime);
static WORK_DEFINE(work_button_pressed, 1, print_button_pressed);

void application_main(void)
{
    gpio_exti_callback(peripherals.user_button, exti_handler);

    work_submit(&work_uptime);
    work_run();
}

static void exti_handler(struct gpio_pin *pin)
{
    ARG_UNUSED(pin);

    work_submit(&work_button_pressed);
}

static void print_button_pressed(struct work *work)
{
    ARG_UNUSED(work);

    const char* message = "button pressed!\r\n";
    uart_write(peripherals.debug_uart, (const uint8_t*) message, strlen(message));
}

static void print_uptime(struct work *work)
{
    ARG_UNUSED(work);

    u64_us_t uptime = system_uptime_get();

    uint32_t uptime_us = uptime % 1000;
    uint32_t uptime_ms = (uptime / 1000) % 1000;
    uint32_t uptime_s = uptime / 1000000;

    char message[64] = {0};

    snprintf(
        message,
        sizeof(message),
        "up-time: %6u.%03u,%03u\r\n",
        (unsigned) uptime_s,
        (unsigned) uptime_ms,
        (unsigned) uptime_us
    );

    uart_write(peripherals.debug_uart, (const uint8_t *) message, strlen(message));

    work_schedule_again(work, 1000);
}
