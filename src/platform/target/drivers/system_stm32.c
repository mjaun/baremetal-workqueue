#include "drivers/system.h"
#include <stm32f4xx_hal.h>

void system_critical_section_enter()
{
    __disable_irq();
}

void system_critical_section_exit()
{
    __enable_irq();
}

u64_ms_t system_uptime_get()
{
    // TODO: Needs to be 64 bit
    return HAL_GetTick();
}

void system_msleep(u32_ms_t timeout)
{
    HAL_Delay(timeout);
}
