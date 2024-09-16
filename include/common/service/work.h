#pragma once

#include "util/types.h"

#ifdef __cplusplus
extern "C" {
#endif

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

/**
 * Defines a new work item.
 *
 * @param _name Name of the defined work item.
 * @param _priority Priority (lower value means higher priority).
 * @param _handler Function to execute the work.
 */
#define WORK_DEFINE(_name, _priority, _handler) \
   struct work _name = { .handler = _handler, .priority = _priority }

/**
 * Enters a loop to execute work items.
 *
 * This might be optimized by going to a low power mode until the next scheduled work
 * item is ready or another item is submitted via ISR.
 */
void work_run(void);

/**
 * Quits the work loop after processing the current work item.
 *
 * This function is supposed to be used for unit testing purposes only.
 */
void work_exit_request(void);

/**
 * Submits an item for execution.
 *
 * If the item is already submitted, this function does nothing.
 * If the item is already scheduled, the schedule is cancelled and it is submitted.
 *
 * If two items are submitted, the item with higher priority is executed first.
 * If two items are submitted with the same priority, the first submitted item is executed first.
 *
 * Note that work items are always executed until completion. This means that an already running low
 * priority item may delay a submitted high priority item.
 *
 * This function is safe to be called from ISRs.
 *
 * @param work Item to submit.
 */
void work_submit(struct work *work);

/**
 * Schedules an item to be submitted after a delay.
 *
 * If the item is already scheduled or submitted, this function does nothing.
 * When scheduled items are submitted, the same rules apply as for `work_submit()`.
 *
 * This function is safe to be called from ISRs.
 *
 * @param work Item to schedule.
 * @param delay Delay in milliseconds.
 */
void work_schedule_after(struct work *work, u32_ms_t delay);

/**
 * Schedules an item to be submitted after a delay relative to the last time it was scheduled.
 *
 * If the item is already scheduled or submitted, this function does nothing.
 * When scheduled items are submitted, the same rules apply as for `work_submit()`.
 *
 * This function is safe to be called from ISRs.
 *
 * @param work Item to schedule.
 * @param delay Delay in milliseconds.
 */
void work_schedule_again(struct work *work, u32_ms_t delay);

/**
 * Schedules an item to be submitted at a specified uptime.
 *
 * If the item is already scheduled or submitted, this function does nothing.
 * When scheduled items are submitted, the same rules apply as for `work_submit()`.
 *
 * This function is safe to be called from ISRs.
 *
 * @param work Item to schedule.
 * @param uptime Delay in milliseconds.
 */
void work_schedule_at(struct work *work, u64_us_t uptime);

/**
 * Removes an item from the submitted or scheduled queue.
 *
 * If the item is not scheduled or submitted, this function does nothing.
 *
 * This function is safe to be called from ISRs.
 *
 * @param work Item to cancel.
 */
void work_cancel(struct work *work);

#ifdef __cplusplus
}
#endif
