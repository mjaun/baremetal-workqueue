#include "service/log.h"
#include "service/system.h"
#include "service/cbprintf.h"
#include "service/work.h"
#include "service/assert.h"
#include <string.h>
#include <stdarg.h>

#define LOG_BUFFER_SIZE          1024
#define LOG_WORK_PRIORITY        10
#define LOG_MAX_MSG_DATA_SIZE    64

#ifdef BUILD_FIRMWARE
#define NEWLINE                  "\r\n"
#else
#define NEWLINE                  "\n"
#endif

#define ANSI_BOLD_RED            "\x1B[1;31m"
#define ANSI_BOLD_YELLOW         "\x1B[1;33m"
#define ANSI_RESET               "\x1B[0m"

/**
 * Ring buffer for log data.
 *
 * Putting data moves the head forward. Getting data moves the tail forward.
 * The indices wrap around if the end of the buffer is reached.
 * If head == tail the buffer is empty. One byte must always left free otherwise this becomes ambigous.
 */
struct log_buffer {
    uint8_t data[LOG_BUFFER_SIZE]; ///< Actual data.
    size_t head; ///< Index of the ring buffer head.
    size_t tail; ///< Index of the ring buffer tail.
    uint32_t dropped; ///< Number of dropped log messages, because there was not enough space.
};

/**
 * Header of a log message.
 *
 * For each log message a header struct followed by the captured format string (see `cbprintf_capture`)
 * is put into the logging ring buffer. The size of both together must not exceed `LOG_MAX_MSG_DATA_SIZE`.
 */
struct log_message_header {
    const struct log_module *module; ///< Module which created this log message.
    u64_us_t timestamp; ///< Timestamp when the log message was created.
    uint8_t level; ///< Log level of this message.
} __attribute__((packed));

static void log_output_handler(struct work *work);
static bool_t log_process(void);

static void ring_buffer_put(const void *data, size_t length);
static size_t ring_buffer_get(void *data);
static uint32_t ring_buffer_read_dropped(void);

static const char *log_level_str(enum log_level level);
static const char *log_level_color(enum log_level level);

static struct log_module *modules;
static struct log_buffer ring_buffer;

WORK_DEFINE(log_output, LOG_WORK_PRIORITY, log_output_handler);

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

void log_panic()
{
    while (log_process()) {
        // process all log messages
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

    size_t package_size = cbvprintf_capture(buffer + sizeof(header), sizeof(buffer) - sizeof(header), format, ap);
    ring_buffer_put(buffer, package_size + sizeof(header));
    work_submit(&log_output);

    va_end(ap);
}

void __log_module_register(struct log_module *module)
{
    module->next = modules;
    modules = module;
}

/**
 * Work handler which processes one log message from the ring buffer.
 * If a message was processed, the work item is resubmitted until there are no more messages.
 *
 * @param work Work item.
 */
static void log_output_handler(struct work *work)
{
    if (log_process()) {
        // resubmit work until there are no more log messages
        work_submit(work);
    }
}

/**
 * Processes one log message from the ring buffer.
 *
 * @return True if a message has been processed, false if the ring buffer was empty.
 */
bool_t log_process(void)
{
    // print number of dropped messages if any
    uint32_t dropped = ring_buffer_read_dropped();

    if (dropped > 0) {
        cbprintf(system_debug_out, ANSI_BOLD_RED "--- %u messages dropped ---" ANSI_RESET NEWLINE, (unsigned) dropped);
    }

    // process one log message
    uint8_t buffer[LOG_MAX_MSG_DATA_SIZE];
    size_t length = ring_buffer_get(buffer);

    if (length < sizeof(struct log_message_header)) {
        RUNTIME_ASSERT(length == 0);
        return false;
    }

    struct log_message_header header;
    memcpy(&header, buffer, sizeof(header));

    uint32_t timestamp_s = header.timestamp / 1000000ULL;
    uint32_t timestamp_us = header.timestamp % 1000000ULL;

    cbprintf(
        system_debug_out,
        "[%02u:%02u:%02u.%03u,%03u] %s<%s> %s: ",
        (unsigned) (timestamp_s / 3600),
        (unsigned) (timestamp_s / 60 % 60),
        (unsigned) (timestamp_s % 60),
        (unsigned) (timestamp_us / 1000),
        (unsigned) (timestamp_us % 1000),
        log_level_color((enum log_level) header.level),
        log_level_str((enum log_level) header.level),
        header.module->name
    );

    cbprintf_restore(
        system_debug_out,
        buffer + sizeof(header),
        length - sizeof(header)
    );

    cbprintf(
        system_debug_out,
        ANSI_RESET NEWLINE
    );

    return true;
}

/**
 * Write log message data into the ring buffer.
 *
 * The log message data must not be larger than `LOG_MAX_MSG_DATA_SIZE`.
 * If there is not enough space in the ring buffer, a dropped counter is incremented and the data is discarded.
 *
 * @param data Data to put.
 * @param length Length in bytes.
 */
static void ring_buffer_put(const void *data, size_t length)
{
    RUNTIME_ASSERT(length <= LOG_MAX_MSG_DATA_SIZE);

    system_critical_section_enter();

    // check if enough space is available
    size_t buffer_free = 0;

    if (ring_buffer.tail > ring_buffer.head) {
        buffer_free = ring_buffer.tail - ring_buffer.head;
    } else {
        buffer_free = LOG_BUFFER_SIZE - (ring_buffer.head - ring_buffer.tail);
    }

    // 1 extra byte for data size
    // 1 extra byte to not have an ambiguous situation if head == tail
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

/**
 * Read log message data from the ring buffer.
 *
 * @param data Buffer to store the read log message data. Must be at least `LOG_MAX_MSG_DATA_SIZE` in size.
 * @return Length of the retrieved message in bytes or 0 if the buffer was empty.
 */
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
            // inconsistency between the read data size byte and the data available in the buffer
            system_critical_section_exit();
            RUNTIME_ASSERT(false);
            return 0;
        }

        ((uint8_t *) data)[i] = ring_buffer.data[ring_buffer.tail];
        ring_buffer.tail = (ring_buffer.tail + 1) % LOG_BUFFER_SIZE;
    }

    system_critical_section_exit();
    return length;
}

/**
 * Reads and resets the dropped message counter.
 *
 * @return Counter value.
 */
static uint32_t ring_buffer_read_dropped(void)
{
    system_critical_section_enter();

    uint32_t ret = ring_buffer.dropped;
    ring_buffer.dropped = 0;

    system_critical_section_exit();
    return ret;
}

/**
 * Get a string representation of the given log level.
 *
 * @param level Log level.
 * @return String representation.
 */
static const char *log_level_str(enum log_level level)
{
    switch (level) {
        case LOG_LEVEL_ERR: return "err";
        case LOG_LEVEL_WRN: return "wrn";
        case LOG_LEVEL_INF: return "inf";
        case LOG_LEVEL_DBG: return "dbg";
        default: return "";
    }
}

/**
 * Get the ANSI escape code for coloring the given log level.
 *
 * @param level Log level.
 * @return ANSI escape code.
 */
static const char *log_level_color(enum log_level level)
{
    switch (level) {
        case LOG_LEVEL_ERR: return ANSI_BOLD_RED;
        case LOG_LEVEL_WRN: return ANSI_BOLD_YELLOW;
        case LOG_LEVEL_INF: return "";
        case LOG_LEVEL_DBG: return "";
        default: return "";
    }
}
