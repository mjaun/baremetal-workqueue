#pragma once

#include <util/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Globally disables interrupts.
 *
 * May be called multiple times like a recursive mutex.
 */
void system_critical_section_enter(void);

/**
 * Globally enables interrupts.
 *
 * May be called multiple times like a recursive mutex.
 */
void system_critical_section_exit(void);

/**
 * Schedules a timer interrupt at the specified uptime.
 *
 * If the timeout is larger than supported by the hardware timer, the timer is scheduled as
 * late as possible and the timer handler is called earlier.
 * If the scheduled time has already passed, the timer is scheduled as early as possible.
 *
 * @param uptime Uptime at which the interrupt shall occur.
 */
void system_timer_schedule_at(u64_ms_t uptime);

/**
 * System timer interrupt handler.
 *
 * This function is not implemented and is supposed to be defined by the work queue.
 */
void system_timer_handler(void);

/**
 * Causes the CPU to enter sleep mode until an interrupt occurs.
 *
 * The function returns once an interrupt is pending, even if it is disabled. Therefore it can be used
 * within a critical section (see `system_critical_section_enter()`) to avoid race conditions.
 * In that case, the pending interrupts are processed after the critical section is exited.
 *
 * A time based wake-up can be scheduled using `system_timer_schedule_at()`.
 */
void system_enter_sleep_mode(void);

/**
 * Returns the system up-time.
 *
 * The up-time timer is started after peripherals have been initialized before calling into
 * the `application_main()` function.
 *
 * @return System up-time in milliseconds.
 */
u64_ms_t system_uptime_get_ms(void);

/**
 * Returns the system up-time.
 *
 * The up-time timer is started after peripherals have been initialized before calling into
 * the `application_main()` function.
 *
 * @return System up-time in microseconds.
 */
u64_us_t system_uptime_get_us(void);

/**
 * Performs a busy sleep for the specified delay.
 *
 * @param delay Delay in milliseconds.
 */
void system_busy_sleep_ms(u64_ms_t delay);

/**
 * Performs a busy sleep for the specified delay.
 *
 * @param delay Delay in microseconds.
 */
void system_busy_sleep_us(u64_us_t delay);

/**
 * Output a character on debug interface.
 */
void system_debug_out(char c);

/**
 * Called if an unrecoverable error occurred.
 *
 * The implementation should use `log_panic` to output any pending log messages and then halt/quit the application.
 */
void system_fatal_error(void);

/**
 * Triggers the software interrupt.
 */
void system_softirq_trigger(void);

/**
 * Software interrupt handler.
 *
 * This function is not implemented and is supposed to be defined by the work queue.
 */
void system_softirq_handler(void);

#ifdef __cplusplus
}
#endif
