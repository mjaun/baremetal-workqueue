#include "services/work.h"
#include "drivers/system.h"

static struct work *submitted_items = NULL;
static struct work *scheduled_items = NULL;

static void process_next_submitted_item();
static void submit_ready_scheduled_items();

static void submit_locked(struct work *item);
static void schedule_locked(struct work *item, uint64_t scheduled_uptime);

static void set_flags(struct work *item, uint32_t flags);
static void clear_flags(struct work *item, uint32_t flags);
static bool_t test_flags_any(struct work *item, uint32_t flags);

/**
 * Enters a loop to execute work items.
 *
 * This function does not return.
 *
 * This might be optimized by going to a low power mode until the next scheduled work
 * item is ready or another item is submitted via ISR.
 */
void work_run()
{
    while (true) {
        submit_ready_scheduled_items();
        process_next_submitted_item();

        system_msleep(1);
    }
}

/**
 * Submits an item for execution.
 *
 * If the item is already SCHEDULED or SUBMITTED, this function does nothing.
 * If two items are submitted, the item with higher priority is executed first.
 * If two items are submitted with the same priority, the first submitted item is executed first.
 * Note that work items are always executed until completion. This means that an already running low
 * priority item may delay a submitted high priority item.
 *
 * This function is safe to be called from ISRs.
 *
 * @param item Item to submit.
 */
void work_submit(struct work *item)
{
    system_critical_section_enter();

    if (!test_flags_any(item, WORK_ITEM_SCHEDULED | WORK_ITEM_SUBMITTED)) {
        submit_locked(item);
    }

    system_critical_section_exit();
}

/**
 * Schedules an item to be submitted after a delay.
 *
 * If the item is already SCHEDULED or SUBMITTED, this function does nothing.
 * When scheduled items are submitted, the same rules apply as for `WorkQueue_Submit`.
 *
 * This function is safe to be called from ISRs.
 *
 * @param item Item to schedule.
 * @param delay_ms Delay in milliseconds.
 */
void work_schedule(struct work *item, uint32_t delay_ms)
{
    uint64_t scheduled_uptime = system_uptime_get() + delay_ms;

    system_critical_section_enter();

    if (!test_flags_any(item, WORK_ITEM_SCHEDULED | WORK_ITEM_SUBMITTED)) {
        schedule_locked(item, scheduled_uptime);
    }

    system_critical_section_exit();
}

/**
 * Submits all work items from the scheduled queue which are ready.
 */
void submit_ready_scheduled_items()
{
    uint64_t current_uptime = system_uptime_get();

    system_critical_section_enter();

    struct work *item = scheduled_items;

    // submit items
    while ((item != NULL) && (item->scheduled_uptime <= current_uptime)) {
        struct work *next = item->next;

        clear_flags(item, WORK_ITEM_SCHEDULED);
        item->scheduled_uptime = 0;
        item->next = NULL;

        submit_locked(item);

        item = next;
    }

    // update queue head
    scheduled_items = item;

    system_critical_section_exit();
}

/**
 * Processes the first queued item, if any.
 */
void process_next_submitted_item()
{
    // remove first item from queue and update state
    system_critical_section_enter();

    struct work *item = submitted_items;

    if (item != NULL) {
        submitted_items = item->next;

        clear_flags(item, WORK_ITEM_SUBMITTED);
        set_flags(item, WORK_ITEM_RUNNING);
        item->next = NULL;
    }

    system_critical_section_exit();

    // process item
    if (item != NULL) {
        item->handler(item);
    }

    // update state
    if (item != NULL) {
        system_critical_section_enter();
        clear_flags(item, WORK_ITEM_RUNNING);
        system_critical_section_exit();
    }
}

/**
 * Helper function to submit a work item. See `WorkQueue_Submit`.
 * Item must not be SCHEDULED or SUBMITTED. Interrupts must be locked.
 */
void submit_locked(struct work *item)
{
    struct work *previous = NULL;
    struct work *next = submitted_items;

    // find correct position
    while (next != NULL) {
        if (next->priority > item->priority) {
            break;
        }

        previous = next;
        next = next->next;
    }

    // insert item
    if (previous != NULL) {
        previous->next = item;
    } else {
        submitted_items = item;
    }

    set_flags(item, WORK_ITEM_SUBMITTED);
    item->next = next;
}

/**
 * Helper function to schedule a work item. See `WorkQueue_Schedule`.
 * Item must not be SCHEDULED or SUBMITTED. Interrupts must be locked.
 */
void schedule_locked(struct work *item, uint64_t scheduled_uptime)
{
    struct work *previous = NULL;
    struct work *next = scheduled_items;

    // find correct position to insert
    while (next != NULL) {
        if (next->scheduled_uptime > item->scheduled_uptime) {
            break;
        }

        previous = next;
        next = next->next;
    }

    // insert item
    if (previous != NULL) {
        previous->next = item;
    } else {
        scheduled_items = item;
    }

    set_flags(item, WORK_ITEM_SCHEDULED);
    item->next = next;
    item->scheduled_uptime = scheduled_uptime;
}

void set_flags(struct work *item, uint32_t flags)
{
    item->flags |= flags;
}

void clear_flags(struct work *item, uint32_t flags)
{
    item->flags &= ~flags;
}

bool_t test_flags_any(struct work *item, uint32_t flags)
{
    return (item->flags & flags) != 0;
}
