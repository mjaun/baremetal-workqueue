#include "drivers/system.h"
#include <unistd.h>

static u64_ms_t uptime_counter;

void system_critical_section_enter()
{

}

void system_critical_section_exit()
{

}

u64_ms_t system_uptime_get()
{
    return uptime_counter;
}

void system_msleep(u32_ms_t timeout)
{
    usleep(timeout * 1000);
    uptime_counter += timeout;
}
