#include "services/work.h"
#include "drivers/system.h"

static struct work *submitted_work = NULL;
static struct work *scheduled_work = NULL;

static void process_next_work();
static void submit_ready_work();
static void sleep_until_ready();

static void submit_add_locked(struct work *work);
static void schedule_add_locked(struct work *work, uint64_t scheduled_uptime);
static void schedule_remove_locked(struct work *work);

static void set_flags(struct work *work, uint32_t flags);
static void clear_flags(struct work *work, uint32_t flags);
static bool_t test_flags_any(struct work *work, uint32_t flags);

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
        submit_ready_work();
        process_next_work();
        sleep_until_ready();
    }
}

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
void work_submit(struct work *work)
{
    system_critical_section_enter();

    // if item is already submitted, do nothing
    if (test_flags_any(work, WORK_ITEM_SUBMITTED)) {
        system_critical_section_exit();
        return;
    }

    // if item is scheduled, remove from schedule queue
    if (test_flags_any(work, WORK_ITEM_SCHEDULED)) {
        schedule_remove_locked(work);
    }

    submit_add_locked(work);

    system_critical_section_exit();
}

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
void work_schedule_after(struct work *work, u32_ms_t delay)
{
    uint64_t scheduled_uptime = system_uptime_get() + (delay * 1000ULL);

    system_critical_section_enter();

    if (!test_flags_any(work, WORK_ITEM_SCHEDULED | WORK_ITEM_SUBMITTED)) {
        schedule_add_locked(work, scheduled_uptime);
    }

    system_critical_section_exit();
}

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
void work_schedule_again(struct work *work, u32_ms_t delay)
{
    uint64_t scheduled_uptime = work->scheduled_uptime + (delay * 1000ULL);

    system_critical_section_enter();

    if (!test_flags_any(work, WORK_ITEM_SCHEDULED | WORK_ITEM_SUBMITTED)) {
        schedule_add_locked(work, scheduled_uptime);
    }

    system_critical_section_exit();
}

/**
 * Removes an item from the scheduled queue.
 *
 * If the item is not scheduled or already submitted, this function does nothing.
 *
 * This function is safe to be called from ISRs.
 *
 * @param work Item to remove from schedule.
 */
void work_schedule_cancel(struct work *work)
{
    system_critical_section_enter();

    if (!test_flags_any(work, WORK_ITEM_SCHEDULED)) {
        schedule_remove_locked(work);
    }

    system_critical_section_exit();
}

/**
 * Submits all work items from the scheduled queue which are ready.
 */
void submit_ready_work()
{
    uint64_t current_uptime = system_uptime_get();

    system_critical_section_enter();

    struct work *work = scheduled_work;

    // submit items
    while ((work != NULL) && (work->scheduled_uptime <= current_uptime)) {
        struct work *next = work->next;

        clear_flags(work, WORK_ITEM_SCHEDULED);
        work->next = NULL;

        submit_add_locked(work);

        work = next;
    }

    // update queue head
    scheduled_work = work;

    system_critical_section_exit();
}

/**
 * Processes the first queued item, if any.
 */
void process_next_work()
{
    // remove first item from queue and update state
    system_critical_section_enter();

    struct work *work = submitted_work;

    if (work != NULL) {
        submitted_work = work->next;

        clear_flags(work, WORK_ITEM_SUBMITTED);
        set_flags(work, WORK_ITEM_RUNNING);
        work->next = NULL;
    }

    system_critical_section_exit();

    // process item
    if (work != NULL) {
        work->handler(work);
    }

    // update state
    if (work != NULL) {
        system_critical_section_enter();
        clear_flags(work, WORK_ITEM_RUNNING);
        system_critical_section_exit();
    }
}

/**
 * Enters sleep mode until the next scheduled work item becomes ready.
 */
void sleep_until_ready()
{
    system_critical_section_enter();

    // don't go to sleep if there is still submitted work
    if (submitted_work != NULL) {
        system_critical_section_exit();
        return;
    }

    if (scheduled_work != NULL) {
        u64_us_t current_uptime = system_uptime_get();

        // don't go to sleep if there is ready work
        if (scheduled_work->scheduled_uptime < current_uptime) {
            system_critical_section_exit();
            return;
        }

        u64_us_t wakeup_timeout = scheduled_work->scheduled_uptime - current_uptime;

        // don't go to sleep if wake-up cannot be scheduled
        if (!system_schedule_wakeup(wakeup_timeout)) {
            system_critical_section_exit();
            return;
        }
    }

    system_enter_sleep_mode();

    system_critical_section_exit();
}

/**
 * Helper function to add a work item to the submitted queue.
 *
 * Item must not be scheduled or submitted.
 * Interrupts must be locked.
 *
 * @param work Item to add.
 */
void submit_add_locked(struct work *work)
{
    struct work *previous = NULL;
    struct work *next = submitted_work;

    // find correct position
    while (next != NULL) {
        if (next->priority > work->priority) {
            break;
        }

        previous = next;
        next = next->next;
    }

    // insert item
    if (previous != NULL) {
        previous->next = work;
    } else {
        submitted_work = work;
    }

    set_flags(work, WORK_ITEM_SUBMITTED);
    work->next = next;
}

/**
 * Helper function to add a work item to the scheduled queue.
 *
 * The item must not be scheduled or submitted.
 * Interrupts must be locked.
 *
 * @param work Item to add.
 * @param scheduled_uptime Uptime at which the item shall be scheduled.
 */
void schedule_add_locked(struct work *work, uint64_t scheduled_uptime)
{
    struct work *previous = NULL;
    struct work *next = scheduled_work;

    // find correct position to insert
    while (next != NULL) {
        if (next->scheduled_uptime > work->scheduled_uptime) {
            break;
        }

        previous = next;
        next = next->next;
    }

    // insert item
    if (previous != NULL) {
        previous->next = work;
    } else {
        scheduled_work = work;
    }

    set_flags(work, WORK_ITEM_SCHEDULED);
    work->next = next;
    work->scheduled_uptime = scheduled_uptime;
}

/**
 * Helper function to remove a work item from the scheduled queue.
 *
 * Interrupts must be locked.
 *
 * @param work Item to remove.
 */
void schedule_remove_locked(struct work *work)
{
    struct work *previous = NULL;
    struct work *next = scheduled_work;

    // find work item
    while ((next != NULL) && (next != work)) {
        previous = next;
        next = next->next;
    }

    // remove item if found
    if (next == work) {
        if (previous != NULL) {
            previous->next = work->next;
        } else {
            scheduled_work = work->next;
        }

        clear_flags(work, WORK_ITEM_SCHEDULED);
        work->next = NULL;
    }
}

/**
 * Helper function to set the specified flags on a work item.
 *
 * @param work Work item.
 * @param flags Flags to set.
 */
void set_flags(struct work *work, uint32_t flags)
{
    work->flags |= flags;
}

/**
 * Helper function to clear the specified flags on a work item.
 *
 * @param work Work item.
 * @param flags Flags to set.
 */
void clear_flags(struct work *work, uint32_t flags)
{
    work->flags &= ~flags;
}

/**
 * Helper function to check if any of the specified flags are set on a work item.
 *
 * @param work Work item.
 * @param flags Flags to check.
 * @return True, if any of the flags is set. False, otherwise.
 */
bool_t test_flags_any(struct work *work, uint32_t flags)
{
    return (work->flags & flags) != 0;
}
