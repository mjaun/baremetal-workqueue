#pragma once

#include "util/types.h"

struct work;

typedef void (*work_handler_t)(struct work *work);

enum work_flags {
    WORK_ITEM_RUNNING = (1 << 0),
    WORK_ITEM_SUBMITTED = (1 << 1),
    WORK_ITEM_SCHEDULED = (1 << 2),
};

struct work {
    work_handler_t handler;
    uint32_t priority;
    u64_us_t scheduled_uptime;
    uint32_t flags;
    struct work *next;
};

#define WORK_DEFINE(_name, _priority, _handler) \
   struct work _name = { .handler = _handler, .priority = _priority }

void work_run();
void work_submit(struct work *work);
void work_schedule_after(struct work *work, u32_ms_t delay);
void work_schedule_again(struct work *work, u32_ms_t delay);
void work_schedule_cancel(struct work *work);
