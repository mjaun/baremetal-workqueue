#pragma once

#include "util/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Globally disables interrupts.
 *
 * May be called multiple times (like a recursive mutex). Interrupts are re-enabled if
 * `system_critical_section_exit()` is called the same amount of times as
 * `system_critical_section_enter()`.
 */
void system_critical_section_enter(void);

/**
 * Globally enables interrupts.
 *
 * May be called multiple times (like a recursive mutex). Interrupts are re-enabled if
 * `system_critical_section_exit()` is called the same amount of times as
 * `system_critical_section_enter()`.
 */
void system_critical_section_exit(void);

/**
 * Schedules a timer interrupt after the specified time.
 *
 * This function is supposed to be used before using `system_enter_sleep_mode()` to
 * schedule a time based wake-up. This should be done within a critical section (see
 * `system_critical_section_enter()`) to avoid race conditions.
 *
 * If the timeout is larger than supported by the hardware timer, the wake-up is scheduled as
 * late as possible and the function still returns true. If the timeout is smaller than supported
 * by the hardware timer, this function does nothing and returns false.
 *
 * @param timeout Timeout after which the interrupt shall occur.
 * @return True, if the wake-up has been scheduled. False, if the timeout is too small to schedule.
 */
bool_t system_schedule_wakeup(u64_ms_t timeout);

/**
 * Causes the CPU to enter sleep mode until an interrupt occurs.
 *
 * This function can be used within a critical section (see `system_critical_section_enter()`)
 * to avoid a race condition. This function returns once an interrupt is pending.
 * The interrupt is processed after the critical section is exited (see `system_critical_section_exit()`).
 *
 * A time based wake-up can be scheduled using `system_schedule_wakeup()`.
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
u64_ms_t system_uptime_ms_get(void);

/**
 * Returns the system up-time.
 *
 * The up-time timer is started after peripherals have been initialized before calling into
 * the `application_main()` function.
 *
 * @return System up-time in microseconds.
 */
u64_us_t system_uptime_us_get(void);

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

void system_softirq_trigger(void);

void system_softirq_handler(void);

#ifdef __cplusplus
}
#endif
