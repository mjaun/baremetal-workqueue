#include "drivers/system.h"
#include <stm32f4xx_hal.h>
#include <main.h>

extern TIM_HandleTypeDef htim2;  ///< 32 bit uptime counter (1 MHz clock)
extern TIM_HandleTypeDef htim3;  ///< 16 bit timer for wakeup (1 kHz clock)

static uint32_t uptime_high32 = 0;
static uint32_t critical_section_depth = 0;

void system_critical_section_enter()
{
    __disable_irq();
    critical_section_depth++;
}

void system_critical_section_exit()
{
    critical_section_depth--;

    if (critical_section_depth == 0) {
        __enable_irq();
    }
}

u64_us_t system_uptime_get()
{
    __disable_irq();

    uint32_t high32 = uptime_high32;
    uint32_t low32 = __HAL_TIM_GetCounter(&htim2);

    // handle overflow while reading
    if (__HAL_TIM_GET_FLAG(&htim2, TIM_IT_UPDATE)) {
        high32++;
        low32 = __HAL_TIM_GetCounter(&htim2);
    }

    __enable_irq();

    return ((u64_us_t) high32 << 32) | (u64_us_t) low32;
}

bool_t system_schedule_wakeup(u64_us_t timeout)
{
    u64_ms_t timeout_ms = timeout / 1000;

    // timeout needs to be at least 1 ms
    if (timeout_ms < 1) {
        return false;
    }

    // timer is 16 bit, if a larger timeout is requested we schedule wake-up as late as we can
    if (timeout_ms > 0xFFFF) {
        timeout_ms = 0xFFFF;
    }

    __HAL_TIM_SetAutoreload(&htim3, timeout_ms - 1);
    __HAL_TIM_SetCounter(&htim3, 0);

    __HAL_TIM_CLEAR_IT(&htim3, TIM_IT_UPDATE);
    __HAL_TIM_ENABLE_IT(&htim3, TIM_IT_UPDATE);
    __HAL_TIM_ENABLE(&htim3);

    return true;
}

void system_enter_sleep_mode()
{
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim2) {
        // uptime counter overflow, increment high register
        uptime_high32++;
    }

    if (htim == &htim3) {
        // wake-up timer expired, disable timer
        __HAL_TIM_DISABLE(&htim3);
        __HAL_TIM_DISABLE_IT(&htim3, TIM_IT_UPDATE);
    }
}
