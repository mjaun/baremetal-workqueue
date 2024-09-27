#include "service/system_sim.h"
#include "service/log.h"
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

static i64_us_t uptime_delta;
static u64_us_t scheduled_wakeup;

static u64_us_t clock_raw_get(void);

void system_critical_section_enter(void)
{
    // do nothing
}

void system_critical_section_exit(void)
{
    // do nothing
}

bool_t system_schedule_wakeup(u64_us_t timeout)
{
    scheduled_wakeup = system_uptime_get() + timeout;
    return true;
}

void system_enter_sleep_mode(void)
{
    if (scheduled_wakeup == 0) {
        // no wakeup scheduled, should not happen
        while (true) {}
    }

    u64_us_t current_uptime = system_uptime_get();

    if (current_uptime < scheduled_wakeup) {
        usleep(scheduled_wakeup - current_uptime);
    }

    scheduled_wakeup = 0;
}

u64_us_t system_uptime_get(void)
{
    return clock_raw_get() - uptime_delta;
}

void system_debug_out(char c)
{
    putchar(c);
}

void system_fatal_error(void)
{
    log_panic();
    exit(1);
}

static u64_us_t clock_raw_get(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (ts.tv_nsec / 1000ULL) + (ts.tv_sec * 1000000ULL);
}

void system_init(void)
{
    uptime_delta = (i64_us_t) clock_raw_get();
}
