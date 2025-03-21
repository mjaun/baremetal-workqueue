#include <service/system.h>
#include <service/log.h>
#include <stm32f4xx_hal.h>
#include <main.h>

extern TIM_HandleTypeDef htim2;  ///< 32 bit uptime counter (1 MHz clock)
extern TIM_HandleTypeDef htim3;  ///< 16 bit timer for wakeup (10 kHz clock)

extern UART_HandleTypeDef huart2;

static uint32_t uptime_high32 = 0;
static uint32_t critical_section_depth = 0;

void system_critical_section_enter(void)
{
    __disable_irq();
    critical_section_depth++;
}

void system_critical_section_exit(void)
{
    critical_section_depth--;

    if (critical_section_depth == 0) {
        __enable_irq();
    }
}

u64_us_t system_uptime_get_us(void)
{
    system_critical_section_enter();

    uint32_t high32 = uptime_high32;
    uint32_t low32 = __HAL_TIM_GetCounter(&htim2);

    // handle overflow while reading
    if (__HAL_TIM_GET_FLAG(&htim2, TIM_IT_UPDATE)) {
        high32++;
        low32 = __HAL_TIM_GetCounter(&htim2);
    }

    system_critical_section_exit();

    return ((u64_us_t) high32 << 32) | (u64_us_t) low32;
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
    u64_us_t until = system_uptime_get_us() + delay;

    while (system_uptime_get_us() < until) {
        // busy sleep
    }
}

void system_wakeup_schedule_at(u64_ms_t uptime)
{
    u64_us_t now = system_uptime_get_us();
    uint64_t timer_period;

    if ((now / 1000) < uptime) {
        timer_period = (uptime * 10) - (now / 100);
    } else {
        // results in smallest reload value 1
        timer_period = 2;
    }

    // timer is 16 bit, if a larger timeout is requested we schedule as late as we can
    if (timer_period > 0x10000) {
        timer_period = 0x10000;
    }

    __HAL_TIM_DISABLE(&htim3);

    __HAL_TIM_SetAutoreload(&htim3, timer_period - 1);
    __HAL_TIM_SetCounter(&htim3, 0);

    __HAL_TIM_CLEAR_IT(&htim3, TIM_IT_UPDATE);
    __HAL_TIM_ENABLE_IT(&htim3, TIM_IT_UPDATE);
    __HAL_TIM_ENABLE(&htim3);
}

void system_enter_sleep_mode(void)
{
    HAL_SuspendTick();
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
    HAL_ResumeTick();
}

void system_debug_out(char c)
{
    HAL_UART_Transmit(&huart2, (const uint8_t*) &c, 1, HAL_MAX_DELAY);
}

void system_fatal_error(void)
{
    log_panic();

    __disable_irq();

    while (true) {
        // halt system
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim2) {
        // uptime counter overflow: increment high register
        uptime_high32++;
    }

    if (htim == &htim3) {
        // system timer expired: disable timer
        __HAL_TIM_DISABLE(&htim3);
        __HAL_TIM_DISABLE_IT(&htim3, TIM_IT_UPDATE);
    }
}
