#include "service/log.h"
#include "service/system.h"
#include "service/print.h"
#include "service/work.h"
#include <string.h>
#include <stdarg.h>

#define LOG_BUFFER_SIZE          1024
#define LOG_MAX_MSG_DATA_SIZE    64

struct log_buffer {
    uint8_t data[LOG_BUFFER_SIZE];
    size_t head;
    size_t tail;
    uint32_t dropped;
};

struct log_message_header {
    const struct log_module *module;
    u64_us_t timestamp;
    uint8_t level;
} __attribute__((packed));

static void log_output(struct work *work);

static void ring_buffer_put(const void *data, size_t length);
static size_t ring_buffer_get(void *data);
static uint32_t ring_buffer_read_dropped();

static const char *log_level_str(enum log_level level);

static struct log_module *modules;
static struct log_buffer ring_buffer;

WORK_DEFINE(log_output_work, 10, log_output);

void log_set_level(const char *module_name, enum log_level level)
{
    struct log_module *module = modules;

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

    struct log_message_header header = {
        .timestamp = system_uptime_get(),
        .level = (uint8_t) level,
        .module = module,
    };

    uint8_t buffer[LOG_MAX_MSG_DATA_SIZE];
    memcpy(buffer, &header, sizeof(header));

    va_list ap;
    va_start(ap, format);

    size_t package_size = print_capture(buffer + sizeof(header), sizeof(buffer) - sizeof(header), format, ap);
    ring_buffer_put(buffer, package_size + sizeof(header));
    work_submit(&log_output_work);

    va_end(ap);
}

void __log_module_register(struct log_module *module)
{
    module->next = modules;
    modules = module;
}

static void log_output(struct work *work)
{
    // print number of dropped messages if any
    uint32_t dropped = ring_buffer_read_dropped();

    if (dropped > 0) {
        print_format(system_debug_out, "<%u messages dropped>\r\n", (unsigned) dropped);
    }

    // process one log message
    uint8_t buffer[LOG_MAX_MSG_DATA_SIZE];
    size_t length = ring_buffer_get(buffer);

    if (length < sizeof(struct log_message_header)) {
        // length should always be zero here
        return;
    }

    struct log_message_header header;
    memcpy(&header, buffer, sizeof(header));

    uint32_t timestamp_s = header.timestamp / 1000000ULL;
    uint32_t timestamp_us = header.timestamp % 1000000ULL;

    print_format(
        system_debug_out,
        "[%02u:%02u:%02u.%03u,%03u] <%s> %s: ",
        (unsigned) (timestamp_s / 3600),
        (unsigned) (timestamp_s / 60 % 60),
        (unsigned) (timestamp_s % 60),
        (unsigned) (timestamp_us / 1000),
        (unsigned) (timestamp_us % 1000),
        log_level_str((enum log_level) header.level),
        header.module->name
    );

    print_output(
        system_debug_out,
        buffer + sizeof(header),
        length - sizeof(header)
    );

    print_format(
        system_debug_out,
        "\r\n"
    );

    // resubmit work until there are no more log messages
    work_submit(work);
}

static void ring_buffer_put(const void *data, size_t length)
{
    system_critical_section_enter();

    // check for illegal data size
    if (length > LOG_MAX_MSG_DATA_SIZE) {
        ring_buffer.dropped++;
        system_critical_section_exit();
        return;
    }

    // check if enough space is left
    size_t buffer_free = 0;

    if (ring_buffer.tail > ring_buffer.head) {
        buffer_free = ring_buffer.tail - ring_buffer.head;
    } else {
        buffer_free = LOG_BUFFER_SIZE - (ring_buffer.head - ring_buffer.tail);
    }

    // 1 extra byte for data size and keep always 1 byte free to not
    // have an ambiguous situation if head == tail
    if (length + 2 > buffer_free) {
        ring_buffer.dropped++;
        system_critical_section_exit();
        return;
    }

    // write data size byte
    ring_buffer.data[ring_buffer.head] = (uint8_t) length;
    ring_buffer.head = (ring_buffer.head + 1) % LOG_BUFFER_SIZE;

    // write data bytes
    for (size_t i = 0; i < length; i++) {
        ring_buffer.data[ring_buffer.head] = ((const uint8_t *) data)[i];
        ring_buffer.head = (ring_buffer.head + 1) % LOG_BUFFER_SIZE;
    }

    system_critical_section_exit();
}

static size_t ring_buffer_get(void *data)
{
    system_critical_section_enter();

    // check if data is available
    if (ring_buffer.head == ring_buffer.tail) {
        system_critical_section_exit();
        return 0;
    }

    // read data size byte
    uint8_t length = ring_buffer.data[ring_buffer.tail];
    ring_buffer.tail = (ring_buffer.tail + 1) % LOG_BUFFER_SIZE;

    // read data bytes
    for (size_t i = 0; i < length; i++) {
        if (ring_buffer.head == ring_buffer.tail) {
            // this should not happen, but in any case we abort to not return garbage data
            system_critical_section_exit();
            return 0;
        }

        ((uint8_t *) data)[i] = ring_buffer.data[ring_buffer.tail];
        ring_buffer.tail = (ring_buffer.tail + 1) % LOG_BUFFER_SIZE;
    }

    system_critical_section_exit();

    return length;
}

static uint32_t ring_buffer_read_dropped()
{
    system_critical_section_enter();

    uint32_t ret = ring_buffer.dropped;
    ring_buffer.dropped = 0;

    system_critical_section_exit();

    return ret;
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
