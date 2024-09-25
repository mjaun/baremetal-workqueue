#include "service/log.h"
#include "service/system.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define DIGITS_BUFFER_SIZE   32

static struct log_module *log_modules;

static void log_printf(const char *format, ...);
static void log_vprintf(const char *format, va_list ap);
static void print_value_signed(int64_t value, uint8_t base);
static void print_value_unsigned(uint64_t value, uint8_t base);
static void print_value_string(const char *str);
static void reverse_string(char *str, int len);
static const char *log_level_str(enum log_level level);

void log_set_level(const char *module_name, enum log_level level)
{
    struct log_module *module = log_modules;

    // find module
    while ((module != NULL) && (strcmp(module->name, module_name) != 0)) {
        module = module->next;
    }

    // update log level
    if (module != NULL) {
        module->level = level;
    }
}

void __log_message(const struct log_module *module, enum log_level level, const char *format, ...)
{
    // early return if log level not enabled
    if (level > module->level) {
        return;
    }

    u64_us_t timestamp = system_uptime_get();

    log_printf(
        "[%u] <%s> %s: ",
        (unsigned) (timestamp / 1000),
        log_level_str(level),
        module->name
    );

    /*
    uint32_t timestamp_s = timestamp / 1000000ULL;
    uint32_t timestamp_us = timestamp % 1000000ULL;

    log_printf(
        "[%02u:%02u:%02u.%03u,%03u] <%s> %s: ",
        (unsigned) (timestamp_s / 3600),
        (unsigned) (timestamp_s / 60 % 60),
        (unsigned) (timestamp_s % 60),
        (unsigned) (timestamp_us / 1000),
        (unsigned) (timestamp_us % 1000),
        log_level_str(level),
        module->name
    );
    */

    va_list ap;
    va_start(ap, format);
    log_vprintf(format, ap);
    va_end(ap);

    system_debug_out('\r');
    system_debug_out('\n');
}

void __log_module_register(struct log_module *module)
{
    module->next = log_modules;
    log_modules = module;
}

void log_printf(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    log_vprintf(format, ap);
    va_end(ap);
}

void log_vprintf(const char *format, va_list ap)
{
    int parse_idx = 0;
    int fspec_idx = -1;

    while (true) {
        char c = format[parse_idx];

        if (c == '\0') {
            // in any case, if we reach the NULL terminator, we quit
            break;
        }

        if (fspec_idx < 0) {
            // we are currently not parsing a format specifier, check for
            // a format specifier, otherwise just print the character
            if (c == '%') {
                fspec_idx = parse_idx;
            } else {
                system_debug_out(c);
            }
        } else {
            // we are currently parsing a format specifier
            switch (c) {
                case 'd':
                case 'i': {
                    int value = va_arg(ap, int);
                    print_value_signed(value, 10);
                    fspec_idx = -1;
                    break;
                }
                case 'u': {
                    unsigned value = va_arg(ap, unsigned);
                    print_value_unsigned(value, 10);
                    fspec_idx = -1;
                    break;
                }
                case 'o': {
                    unsigned value = va_arg(ap, unsigned);
                    print_value_unsigned(value, 8);
                    fspec_idx = -1;
                    break;
                }
                case 'x': {
                    unsigned value = va_arg(ap, unsigned);
                    print_value_unsigned(value, 16);
                    fspec_idx = -1;
                    break;
                }
                case 'p': {
                    uintptr_t value = (uintptr_t) va_arg(ap, const void *);
                    print_value_unsigned(value, 16);
                    fspec_idx = -1;
                    break;
                }
                case 'c': {
                    char value = (char) va_arg(ap, int);
                    system_debug_out(value);
                    fspec_idx = -1;
                    break;
                }
                case 's': {
                    const char *value = va_arg(ap, const char *);
                    print_value_string(value);
                    fspec_idx = -1;
                    break;
                }
                case '%': {
                    system_debug_out('%');
                    fspec_idx = -1;
                    break;
                }
                default: {
                    // unsupported format specifier, print as is
                    for (int i = fspec_idx; i <= parse_idx; i++) {
                        system_debug_out(format[i]);
                    }
                    fspec_idx = -1;
                    break;
                }
            }
        }

        parse_idx++;
    }
}

static void print_value_signed(int64_t value, uint8_t base)
{
    if (value < 0) {
        system_debug_out('-');
        print_value_unsigned((uint64_t) -value, base);
    } else {
        print_value_unsigned((uint64_t) value, base);
    }
}

static void print_value_unsigned(uint64_t value, uint8_t base)
{
    if (value == 0) {
        system_debug_out('0');
        return;
    }

    char buf[DIGITS_BUFFER_SIZE];
    int idx = 0;

    while (value > 0) {
        uint8_t c = (uint8_t) (value % base);

        if (c >= 10) {
            c = (uint8_t) 'a' + c - 10;
        } else {
            c = (uint8_t) '0' + c;
        }

        buf[idx] = (char) c;
        idx++;

        value /= base;
    }

    buf[idx] = '\0';

    reverse_string(buf, idx);
    print_value_string(buf);
}

void print_value_string(const char *str)
{
    for (int i = 0; str[i] != '\0'; i++) {
        system_debug_out(str[i]);
    }
}

static void reverse_string(char *str, int len)
{
    int start = 0;
    int end = len - 1;

    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;

        start++;
        end--;
    }
}

static const char *log_level_str(enum log_level level)
{
    switch (level) {
        case LOG_LEVEL_ERR: return "err";
        case LOG_LEVEL_WRN: return "wrn";
        case LOG_LEVEL_INF: return "inf";
        case LOG_LEVEL_DBG: return "dbg";
        default: return "?";
    }
}
