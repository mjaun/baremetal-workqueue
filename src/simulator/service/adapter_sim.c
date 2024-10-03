#include <service/adapter_sim.h>
#include <service/assert.h>
#include <service/work.h>
#include <service/log.h>
#include <util/unused.h>

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <semaphore.h>
#include <cjson/cJSON.h>

#define ADAPTER_PRIORITY       20
#define MESSAGE_BUFFER_SIZE    1024

LOG_MODULE_REGISTER(adapter_sim);

struct adapter_message {
    cJSON *json;
};

static void *thread_handler(void *arg);
static void handle_connection(int client_sock);
static void parse_request(const uint8_t *request, size_t length);
static void process_request(struct work *work);
static char *vprintf_alloc(const char *format, va_list ap);

static struct adapter *adapter_list;

static cJSON *current_request;
static sem_t request_processed;

WORK_DEFINE(process_request_work, ADAPTER_PRIORITY, process_request);

void adapter_setup()
{
    int ret = sem_init(&request_processed, 0, 1);
    RUNTIME_ASSERT(ret == 0);

    pthread_t thread;

    ret = pthread_create(&thread, NULL, thread_handler, NULL);
    RUNTIME_ASSERT(ret == 0);

    ret = pthread_detach(thread);
    RUNTIME_ASSERT(ret == 0);
}

void adapter_init(struct adapter *adapter, adapter_message_handler_t handler, const char *topic_format, ...)
{
    va_list ap;
    va_start(ap, topic_format);
    const char *topic = vprintf_alloc(topic_format, ap);
    va_end(ap);

    adapter->topic = topic;
    adapter->handler = handler;
    adapter->next = adapter_list;
    adapter_list = adapter;
}

void adapter_send_void(struct adapter *adapter)
{
    ARG_UNUSED(adapter);
    // not implemented yet
}

void adapter_send_bool(struct adapter *adapter, bool_t value)
{
    ARG_UNUSED(adapter);
    ARG_UNUSED(value);
    // not implemented yet
}

void adapter_send_string(struct adapter *adapter, const char *value)
{
    ARG_UNUSED(adapter);
    ARG_UNUSED(value);
    // not implemented yet
}

bool_t adapter_check_bool(const struct adapter_message *message, bool_t expected)
{
    if (!cJSON_IsBool(message->json)) {
        return false;
    }

    return cJSON_IsTrue(message->json) == expected;
}

bool_t adapter_check_string(const struct adapter_message *message, const char *expected)
{
    if (!cJSON_IsString(message->json)) {
        return false;
    }

    return strcmp(cJSON_GetStringValue(message->json), expected) == 0;
}

static void *thread_handler(void *arg)
{
    ARG_UNUSED(arg);

    LOG_INF("Adapter thread started.");

    int ret, server_sock, client_sock;
    struct sockaddr_un server_addr, client_addr;

    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    RUNTIME_ASSERT(server_sock >= 0);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    snprintf(server_addr.sun_path, sizeof(server_addr.sun_path), "/tmp/%d.socket", getpid());

    unlink(server_addr.sun_path);

    ret = bind(server_sock, (struct sockaddr *) &server_addr, sizeof(server_addr));
    RUNTIME_ASSERT(ret == 0);

    ret = listen(server_sock, 0);
    RUNTIME_ASSERT(ret == 0);

    while (true) {
        LOG_INF("Waiting for connection...");

        socklen_t client_addr_len = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr *) &client_addr, &client_addr_len);

        if (client_sock < 0) {
            LOG_ERR("Accepting connection failed.");
            continue;
        }

        handle_connection(client_sock);
    }

    return NULL;
}

void handle_connection(int client_sock)
{
    LOG_INF("Client connected.");

    while (true) {
        uint8_t request[MESSAGE_BUFFER_SIZE] = {0};
        int length = read(client_sock, request, sizeof(request) - 1);
        RUNTIME_ASSERT(length >= 0);

        if (length < 1) {
            LOG_INF("Client disconnected.");
            close(client_sock);
            break;
        }

        LOG_DBG("%d bytes received.", length);
        parse_request(request, length);
    }
}

void parse_request(const uint8_t *request, size_t length)
{
    cJSON *json = cJSON_ParseWithLength((const char *) request, length);

    int ret = sem_wait(&request_processed);
    RUNTIME_ASSERT(ret == 0);

    current_request = json;
    work_submit(&process_request_work);
}

void process_request(struct work *work)
{
    ARG_UNUSED(work);

    const char *topic = cJSON_GetStringValue(cJSON_GetObjectItem(current_request, "topic"));

    if (topic == NULL) {
        LOG_ERR("Invalid request.");
        goto cleanup;
    }

    struct adapter *adapter = adapter_list;

    while (adapter != NULL) {
        if (strcmp(adapter->topic, topic) == 0) {
            break;
        }

        adapter = adapter->next;
    }

    if ((adapter != NULL) && (adapter->handler != NULL)) {
        struct adapter_message message = {
            .json = cJSON_GetObjectItem(current_request, "message"),
        };

        adapter->handler(adapter, &message);
    } else {
        LOG_WRN("Unhandled request.");
    }

cleanup:
    cJSON_Delete(current_request);
    current_request = NULL;

    int ret = sem_post(&request_processed);
    RUNTIME_ASSERT(ret == 0);
}

char *vprintf_alloc(const char *format, va_list ap)
{
    va_list ap_copy;
    va_copy(ap_copy, ap);

    int len = vsnprintf(NULL, 0, format, ap_copy);
    RUNTIME_ASSERT(len > 0);

    char *str = (char *) malloc(len + 1);
    vsnprintf(str, len + 1, format, ap);
    return str;
}
