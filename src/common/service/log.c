#include "service/log.h"
#include "service/system.h"
#include "util/unused.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define MESSAGE_BUFFER_SIZE      128

static struct log_module *log_modules;

static const char* log_level_str(enum log_level level);
static void log_printf(const char *format, ...);
static void log_vprintf(const char *format, va_list ap);

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

	va_list ap;
	va_start(ap, format);
    log_vprintf(format, ap);
	va_end(ap);

    system_debug_out('\r');
    system_debug_out('\n');
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
    char buffer[MESSAGE_BUFFER_SIZE];
    int length = vsnprintf(buffer, sizeof(buffer), format, ap);

    if (length < 0) {
        return;
    }

    // truncate message if too long, last character is always a null terminator which we don't use
    if (length >= MESSAGE_BUFFER_SIZE) {
        length = MESSAGE_BUFFER_SIZE - 1;
    }

    for (int i = 0; i < length; i++) {
        system_debug_out(buffer[i]);
    }
}

void __log_module_register(struct log_module *module)
{
    module->next = log_modules;
    log_modules = module;
}

const char * log_level_str(enum log_level level)
{
    switch (level) {
        case LOG_LEVEL_ERR: return "err";
        case LOG_LEVEL_WRN: return "wrn";
        case LOG_LEVEL_INF: return "inf";
        case LOG_LEVEL_DBG: return "dbg";
        default: return "?";
    }
}
