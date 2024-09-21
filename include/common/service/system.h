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
void system_critical_section_enter();

/**
 * Globally enables interrupts.
 *
 * May be called multiple times (like a recursive mutex). Interrupts are re-enabled if
 * `system_critical_section_exit()` is called the same amount of times as
 * `system_critical_section_enter()`.
 */
void system_critical_section_exit();

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
bool_t system_schedule_wakeup(u64_us_t timeout);

/**
 * Causes the CPU to enter sleep mode until an interrupt occurs.
 *
 * This function can be used within a critical section (see `system_critical_section_enter()`)
 * to avoid a race condition. This function returns once an interrupt is pending.
 * The interrupt is processed after the critical section is exited (see `system_critical_section_exit()`).
 *
 * A time based wake-up can be scheduled using `system_schedule_wakeup()`.
 */
void system_enter_sleep_mode();

/**
 * Returns the system up-time in microseconds.
 *
 * The up-time timer is started after peripherals have been initialized before calling into
 * the `application_main()` function.
 *
 * @return System up-time in microseconds.
 */
u64_us_t system_uptime_get();

/**
 * Output a character on debug interface.
 */
void system_debug_out(char c);

#ifdef __cplusplus
}
#endif
