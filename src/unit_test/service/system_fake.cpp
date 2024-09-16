#include "CppUTest/CommandLineTestRunner.h"
#include "service/system.h"

#define TICK_DURATION_US    1000

static u64_us_t uptime_counter;
static u64_us_t scheduled_wakeup;

extern "C" void system_critical_section_enter(void)
{
    // do nothing
}

extern "C" void system_critical_section_exit(void)
{
    // do nothing
}

extern "C" bool_t system_schedule_wakeup(u64_us_t timeout)
{
    scheduled_wakeup = uptime_counter + timeout;
    return true;
}

extern "C" void system_enter_sleep_mode(void)
{
    while (true) {
        if ((scheduled_wakeup != 0) && (uptime_counter >= scheduled_wakeup)) {
            break;
        }

        // in future simulated interrupts can cause a break here as well

        uptime_counter += TICK_DURATION_US;
    }

    scheduled_wakeup = 0;
}

extern "C" u64_us_t system_uptime_get(void)
{
    return uptime_counter;
}

int main(int argc, char* argv[])
{
    return RUN_ALL_TESTS(argc, argv);
}
