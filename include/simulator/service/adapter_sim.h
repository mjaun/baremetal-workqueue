#pragma once

#include <util/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct adapter;
struct adapter_message;

typedef void (*adapter_message_handler_t)(struct adapter *adapter, const struct adapter_message *message);

struct adapter {
    const char *topic;
    adapter_message_handler_t handler;
    struct adapter *next;
};

void adapter_setup();

void adapter_init(struct adapter *adapter, adapter_message_handler_t handler, const char *topic_format, ...);

void adapter_send_void(struct adapter *adapter);

void adapter_send_bool(struct adapter *adapter, bool_t value);

void adapter_send_string(struct adapter *adapter, const char *value);

bool_t adapter_check_bool(const struct adapter_message *message, bool_t expected);

bool_t adapter_check_string(const struct adapter_message *message, const char *expected);

#ifdef __cplusplus
}
#endif
