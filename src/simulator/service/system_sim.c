#include <service/system_sim.h>
#include <service/assert.h>
#include <service/log.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <pthread.h>

static i64_us_t uptime_delta;
static u64_us_t scheduled_wakeup;

static pthread_mutex_t critical_section_mutex;

static u64_us_t clock_raw_get(void);

void system_setup(void)
{
    uptime_delta = (i64_us_t) clock_raw_get();

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    int ret = pthread_mutex_init(&critical_section_mutex, &attr);
    RUNTIME_ASSERT(ret == 0);
}

void system_critical_section_enter(void)
{
    int ret = pthread_mutex_lock(&critical_section_mutex);
    RUNTIME_ASSERT(ret == 0);
}

void system_critical_section_exit(void)
{
    int ret = pthread_mutex_unlock(&critical_section_mutex);
    RUNTIME_ASSERT(ret == 0);
}

void system_wakeup_schedule_at(u64_ms_t uptime)
{
    scheduled_wakeup = uptime * 1000;
}

void system_enter_sleep_mode(void)
{
    u64_us_t current_uptime = system_uptime_get_us();

    if (current_uptime < scheduled_wakeup) {
        usleep(scheduled_wakeup - current_uptime);
    }

    scheduled_wakeup = 0;
}

u64_us_t system_uptime_get_us(void)
{
    return clock_raw_get() - uptime_delta;
}

u64_ms_t system_uptime_get_ms(void)
{
    return system_uptime_get_us() / 1000;
}

void system_busy_sleep_ms(u64_ms_t delay)
{
    system_busy_sleep_us(delay * 1000);
}

void system_busy_sleep_us(u64_us_t delay)
{
    usleep(delay);
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
