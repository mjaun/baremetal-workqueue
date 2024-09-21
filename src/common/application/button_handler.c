#include "application/peripherals.h"
#include "service/work.h"
#include "driver/gpio.h"
#include "service/log.h"
#include "util/unused.h"

LOG_MODULE_REGISTER(button_handler);

static void exti_handler(struct gpio_pin *pin);

static void print_button_pressed(struct work *work);

static WORK_DEFINE(work_button_pressed, 1, print_button_pressed);

void button_handler_init(void)
{
    gpio_exti_callback(peripherals.user_button, exti_handler);
}

static void exti_handler(struct gpio_pin *pin)
{
    ARG_UNUSED(pin);

    work_submit(&work_button_pressed);
}

static void print_button_pressed(struct work *work)
{
    ARG_UNUSED(work);

    LOG_INF("button pressed!");
}
