#include "service/system_sim.h"
#include "service/assert.h"
#include "service/log.h"
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

static u64_us_t current_uptime; ///< Our simulated uptime.
static i64_us_t uptime_target_delta; ///< Target difference between the monotonic clock and our simulated uptime.
static u64_us_t scheduled_wakeup; ///< Delay until scheduled wakeup when entering sleep.

static u64_us_t clock_raw_get(void);


void system_init(void)
{
    uptime_target_delta = (i64_us_t) clock_raw_get();
}

void system_critical_section_enter(void)
{
    // do nothing
}

void system_critical_section_exit(void)
{
    // do nothing
}

bool_t system_schedule_wakeup(u64_ms_t timeout)
{
    scheduled_wakeup = timeout * 1000;
    return true;
}

void system_enter_sleep_mode(void)
{
    // not actual busy sleep, simulates time and performs OS sleep
    system_busy_sleep_us(scheduled_wakeup);
}

u64_us_t system_uptime_us_get(void)
{
    return current_uptime;
}

u64_ms_t system_uptime_ms_get(void)
{
    return current_uptime / 1000;
}

void system_busy_sleep_ms(u64_ms_t delay)
{
    system_busy_sleep_us(delay * 1000);
}

void system_busy_sleep_us(u64_us_t delay)
{
    // move simulated time forward
    current_uptime += delay;
    i64_us_t uptime_delta = clock_raw_get() - current_uptime;

    // sleep if we are ahead of time
    if (uptime_delta < uptime_target_delta) {
        usleep(uptime_target_delta - uptime_delta);
    }
}

void system_softirq_trigger(void)
{
    // immediately call into handler
    system_softirq_handler();
}

__attribute__((weak)) void system_softirq_handler(void)
{
    // supposed to be overridden, do nothing
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
