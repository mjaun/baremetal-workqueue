#pragma once

#define DEFAULT_LOG_LEVEL  LOG_LEVEL_INF

/**
 * Log levels.
 */
enum log_level {
    LOG_LEVEL_ERR = 0, ///< Error
    LOG_LEVEL_WRN = 1, ///< Warning
    LOG_LEVEL_INF = 2, ///< Information
    LOG_LEVEL_DBG = 3, ///< Debug message
};

/**
 * Log module information.
 */
struct log_module {
    const char *name; ///< Name of this module.
    enum log_level level; ///< Minimum log level of this module.
    struct log_module *next; ///< Next module to form a linked list of all registered modules.
};

/**
 * Registers a new log module.
 *
 * @param _name Name of the module.
 */
#define LOG_MODULE_REGISTER(_name) \
    static struct log_module __log_module = { \
        .name = #_name, \
        .level = DEFAULT_LOG_LEVEL, \
    }; \
    static void __attribute__((constructor)) __log_module_register_this(void) { \
        __log_module_register(&__log_module); \
    } \
    static struct log_module __log_module \

/**
 * Logs an error message.
 *
 * The macro takes printf style arguments. It is safe to use from ISRs.
 * The provided information is recorded and outputted later by the logging work item.
 */
#define LOG_ERR(...) __log_message(&__log_module, LOG_LEVEL_ERR, __VA_ARGS__)

/**
 * Logs a warning message.
 *
 * The macro takes printf style arguments. It is safe to use from ISRs.
 * The provided information is recorded and outputted later by the logging work item.
 */
#define LOG_WRN(...) __log_message(&__log_module, LOG_LEVEL_WRN, __VA_ARGS__)

/**
 * Logs an info message.
 *
 * The macro takes printf style arguments. It is safe to use from ISRs.
 * The provided information is recorded and outputted later by the logging work item.
 */
#define LOG_INF(...) __log_message(&__log_module, LOG_LEVEL_INF, __VA_ARGS__)

/**
 * Logs a debug message.
 *
 * The macro takes printf style arguments. It is safe to use from ISRs.
 * The provided information is recorded and outputted later by the logging work item.
 */
#define LOG_DBG(...) __log_message(&__log_module, LOG_LEVEL_DBG, __VA_ARGS__)

/**
 * Changes the log level for a module.
 *
 * @param module_name Name of the module to adjust the log level.
 * @param level New log level for the module.
 */
void log_set_level(const char *module_name, enum log_level level);

/**
 * Immediately flush all pending log messages.
 *
 * If called from an ISRs, this might interfere with the log work item causing fragmented log output.
 */
void log_panic();

/**
 * Creates a log message and writes it to the ring buffer.
 * Submits the log handler work item to process the message.
 *
 * This function is safe to use from ISRs.
 * This function is intended for internal use by logging macros.
 *
 * @param module Module sending to log message.
 * @param level Log level of the message.
 * @param format Format string.
 * @param ... Arguments for the format string.
 */
void __log_message(const struct log_module *module, enum log_level level, const char *format, ...);

/**
 * Registers a new module.
 *
 * This function is intended for internal use by logging macros.
 */
void __log_module_register(struct log_module *module);
