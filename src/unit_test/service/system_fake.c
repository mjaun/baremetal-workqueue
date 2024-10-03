#include <service/system.h>
#include <service/assert.h>
#include <util/unused.h>

static u64_us_t uptime_counter;
static u64_us_t scheduled_wakeup;

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
    if (scheduled_wakeup == 0) {
        // no wakeup scheduled, should not happen
        RUNTIME_ASSERT(false);
        return;
    }

    uptime_counter = scheduled_wakeup;
    scheduled_wakeup = 0;
}

u64_us_t system_uptime_get(void)
{
    return uptime_counter;
}

void system_debug_out(char c)
{
    // do nothing
    ARG_UNUSED(c);
}
