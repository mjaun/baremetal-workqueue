#include <service/system.h>
#include <service/unit_test.h>
#include <util/unused.h>

static bool soft_irq_triggered;
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

bool_t system_schedule_wakeup(u64_us_t timeout)
{
    scheduled_wakeup = uptime_counter + timeout;
    return true;
}

void system_enter_sleep_mode(void)
{
    if (soft_irq_triggered) {
        soft_irq_triggered = false;
        system_softirq_handler();
    }

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
    soft_irq_triggered = true;
}

__attribute__((weak)) void system_softirq_handler(void)
{
    // supposed to be overridden, do nothing
}

void system_debug_out(char c)
{
    static char output_buffer[256];
    static size_t output_index;
    static size_t line_counter;

    if (c == '\n') {
        line_counter++;
        output_buffer[output_index] = '\0';
        ut_print_c_location(output_buffer, "DEBUG", line_counter);
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

void system_fatal_error(void)
{
    mock_c()->actualCall("system_fatal_error");
}
