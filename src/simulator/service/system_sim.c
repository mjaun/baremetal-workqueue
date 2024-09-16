#include "service/system_sim.h"
#include "util/unused.h"
#include <unistd.h>
#include <time.h>

#define TICK_DURATION_US    1000

static u64_us_t uptime_counter;
static i64_us_t uptime_target_delta;
static u64_us_t scheduled_wakeup;

static u64_us_t clock_raw_get(void);
static i64_us_t clock_current_delta_get(void);

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
    scheduled_wakeup = uptime_counter + timeout;
    return true;
}

void system_enter_sleep_mode(void)
{
    while (true) {
        if ((scheduled_wakeup != 0) && (uptime_counter >= scheduled_wakeup)) {
            break;
        }

        // in future simulated interrupts can cause a break here as well

        uptime_counter += TICK_DURATION_US;

        i64_us_t current_delta = clock_current_delta_get();

        if (current_delta < uptime_target_delta) {
            usleep(uptime_target_delta - current_delta);
        }
    }

    scheduled_wakeup = 0;
}

u64_us_t system_uptime_get(void)
{
    return uptime_counter;
}

static i64_us_t clock_current_delta_get(void)
{
    return clock_raw_get() - uptime_counter;
}

static u64_us_t clock_raw_get(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (ts.tv_nsec / 1000ULL) + (ts.tv_sec * 1000000ULL);
}

void system_init(void)
{
    uptime_target_delta = (i64_us_t) clock_raw_get();
}
