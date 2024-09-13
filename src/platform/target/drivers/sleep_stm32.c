#include "drivers/sleep.h"
#include <stm32f4xx_hal.h>

void msleep(u32_ms_t timeout)
{
    HAL_Delay(timeout);
}
