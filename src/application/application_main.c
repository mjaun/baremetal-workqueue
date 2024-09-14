#include "application/peripherals.h"
#include "services/work.h"
#include "drivers/gpio.h"
#include "drivers/uart.h"
#include "util/unused.h"
#include <string.h>

static void print_hello_once(struct work *work);
static void toogle_led_forever(struct work *work);

static WORK_ITEM_DEFINE(print_hello, 1, print_hello_once);
static WORK_ITEM_DEFINE(toggle_led, 1, toogle_led_forever);

void application_main(void)
{
    work_submit(&print_hello);
    work_submit(&toggle_led);
    work_run();
}

static void print_hello_once(struct work *work)
{
    ARG_UNUSED(work);

    const char *message = "Hello World!\r\n";
    uart_write(peripherals.debug_uart, (const uint8_t *) message, strlen(message));
}

static void toogle_led_forever(struct work *work)
{
    gpio_toggle(peripherals.user_led);
    work_schedule(work, 500);
}
