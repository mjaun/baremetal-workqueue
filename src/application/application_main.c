#include "application/peripherals.h"
#include "drivers/gpio.h"
#include "drivers/uart.h"
#include "drivers/sleep.h"
#include <string.h>

void application_main(void)
{
    const char* message = "hello world!\r\n";
    uart_write(peripherals.debug_uart, (const uint8_t *) message, strlen(message));

    while (1) {
        gpio_toggle(peripherals.user_led);
        msleep(500);
    }
}
