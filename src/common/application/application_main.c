#include "application/application_startup.h"
#include "application/peripherals.h"
#include "driver/gpio.h"
#include "service/work.h"
#include "service/system.h"
#include "service/log.h"
#include "util/unused.h"

LOG_MODULE_REGISTER(application_main);

static void exti_handler(struct gpio_pin *pin);
static void high_prio_handler(struct work *work);
static void low_prio_handler(struct work *work);

WORK_DEFINE(high_prio, -5, high_prio_handler);
WORK_DEFINE(low_prio, 5, low_prio_handler);

void application_main(void)
{
    gpio_exti_callback(peripherals.user_button, exti_handler);

    work_submit(&low_prio);

    work_run();
}

static void exti_handler(struct gpio_pin *pin)
{
    ARG_UNUSED(pin);

    u64_us_t start = system_uptime_us_get();
    LOG_ERR("Button ISR: %04u %s!", 42, "argtest");
    u64_us_t end = system_uptime_us_get();

    work_submit(&high_prio);

    LOG_INF("Logging took %u us", (unsigned) (end - start));
}

void high_prio_handler(struct work *work)
{
    ARG_UNUSED(work);

    LOG_WRN("HIGH start");

    u64_ms_t end = system_uptime_ms_get() + 500;
    while (system_uptime_ms_get() < end) {
        // busy sleep
    }

    LOG_WRN("HIGH done");
}

void low_prio_handler(struct work *work)
{
    ARG_UNUSED(work);

    work_schedule_again(&low_prio, 2000);

    LOG_INF("LOW start");

    u64_us_t end = system_uptime_ms_get() + 1000;
    while (system_uptime_ms_get() < end) {
        // busy sleep
    }

    LOG_INF("LOW done");
}
