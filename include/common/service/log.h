#pragma once

#define DEFAULT_LOG_LEVEL  LOG_LEVEL_INF

enum log_level {
    LOG_LEVEL_ERR = 0,
    LOG_LEVEL_WRN = 1,
    LOG_LEVEL_INF = 2,
    LOG_LEVEL_DBG = 3,
};

struct log_module {
    const char *name;
    enum log_level level;
    struct log_module *next;
};

#define LOG_MODULE_REGISTER(_name) \
    static struct log_module __log_module = { \
        .name = #_name, \
        .level = DEFAULT_LOG_LEVEL, \
    }; \
    static void __attribute__((constructor)) __log_module_register_this(void) { \
        __log_module_register(&__log_module); \
    } \
    static struct log_module __log_module \

#define LOG_ERR(...) __log_message(&__log_module, LOG_LEVEL_ERR, __VA_ARGS__)
#define LOG_WRN(...) __log_message(&__log_module, LOG_LEVEL_WRN, __VA_ARGS__)
#define LOG_INF(...) __log_message(&__log_module, LOG_LEVEL_INF, __VA_ARGS__)
#define LOG_DBG(...) __log_message(&__log_module, LOG_LEVEL_DBG, __VA_ARGS__)

/**
 * Changes the log level for a module.
 *
 * @param module_name Name of the module to adjust the log level.
 * @param level New log level for the module.
 */
void log_set_level(const char *module_name, enum log_level level);

/**
 * Logs a message.
 *
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
