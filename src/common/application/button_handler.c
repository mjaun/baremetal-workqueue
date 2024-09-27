#include "application/peripherals.h"
#include "service/work.h"
#include "driver/gpio.h"
#include "service/log.h"
#include "service/assert.h"
#include "service/system.h"
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

    u64_us_t start = system_uptime_get();
    LOG_INF("button pressed ISR: %u %s!", 42, "test");
    u64_us_t end = system_uptime_get();

    LOG_INF("took %u us", (unsigned) (end - start));

    work_submit(&work_button_pressed);
}

static void print_button_pressed(struct work *work)
{
    ARG_UNUSED(work);

    LOG_INF("button pressed work!");

    RUNTIME_ASSERT(false);
}
