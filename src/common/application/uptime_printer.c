#include "application/peripherals.h"
#include "driver/uart.h"
#include "service/work.h"
#include "service/system.h"
#include "util/unused.h"
#include <string.h>
#include <stdio.h>

static void print_uptime(struct work *work);

static WORK_DEFINE(work_uptime, 1, print_uptime);

void uptime_printer_init(void)
{
    work_submit(&work_uptime);
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
