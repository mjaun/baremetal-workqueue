#include <service/assert.h>
#include <service/system.h>
#include <service/unit_test.h>

static bool soft_irq_active;
static bool soft_irq_requested;
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

bool_t system_schedule_wakeup(u64_ms_t timeout)
{
    scheduled_wakeup = uptime_counter + (timeout * 1000);
    return true;
}

void system_enter_sleep_mode(void)
{
    RUNTIME_ASSERT(scheduled_wakeup != 0);
    uptime_counter = scheduled_wakeup;
    scheduled_wakeup = 0;
}

u64_us_t system_uptime_get_us(void)
{
    return uptime_counter;
}

u64_ms_t system_uptime_get_ms(void)
{
    return uptime_counter / 1000;
}

void system_softirq_trigger(void)
{
    if (!soft_irq_active) {
        soft_irq_active = true;

        do {
            soft_irq_requested = false;
            system_softirq_handler();
        } while (soft_irq_requested);

        soft_irq_active = false;
    } else {
        // interrupt triggered while ISR still active, remember flag
        // and handle interrupt again after exiting ISR in loop above
        soft_irq_requested = true;
    }
}

__attribute__((weak)) void system_softirq_handler(void)
{
    // supposed to be overridden, do nothing
}

__attribute__((weak)) void system_debug_out(char c)
{
    static char output_buffer[256];
    static size_t output_index;
    static size_t line_counter;

    if (c == '\n') {
        line_counter++;
        output_buffer[output_index] = '\0';
        ut_print_c_location(output_buffer, "OUT", line_counter);
        output_index = 0;
    } else {
        output_buffer[output_index] = c;
        output_index++;

        if (output_index >= sizeof(output_buffer)) {
            output_index = 0;
            FAIL_TEXT_C("Output buffer overflow!");
        }
    }
}

void system_busy_sleep_ms(u64_ms_t delay)
{
    system_busy_sleep_us(delay * 1000);
}

void system_busy_sleep_us(u64_us_t delay)
{
    uptime_counter += delay;
}

void system_fatal_error(void)
{
    mock_c()->actualCall("system_fatal_error");
}
