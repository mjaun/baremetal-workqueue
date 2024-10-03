#include <service/work.h>
#include <service/system.h>
#include <service/assert.h>
#include <util/unused.h>

static volatile bool_t running = false;
static struct work *high_prio_queue = NULL;
static struct work *low_prio_queue = NULL;
static struct work *scheduled_queue = NULL;

static bool_t process_next_work(struct work **queue);
static void submit_ready_work();
static void sleep_until_ready();

static void submit_add_locked(struct work **queue, struct work *work);
static void schedule_add_locked(struct work *work, u64_ms_t scheduled_uptime);
static void remove_locked(struct work **queue, struct work *work, uint32_t flags_to_clear);

static void set_flags(struct work *work, uint32_t flags);
static void clear_flags(struct work *work, uint32_t flags);
static bool_t test_flags_any(struct work *work, uint32_t flags);

void work_run(void)
{
    running = true;

    // trigger softirq in case there is already high prio work
    system_softirq_trigger();

    // process low prio queue and scheduled work
    while (running) {
        submit_ready_work();

        if (!process_next_work(&low_prio_queue)) {
            sleep_until_ready();
        }
    }
}

void system_softirq_handler(void)
{
    // process high prio queue
    while (running) {
        if (!process_next_work(&high_prio_queue)) {
            break;
        }
    }
}

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
        remove_locked(&scheduled_queue, work, WORK_ITEM_SCHEDULED);
    }

    if (work->priority >= 0) {
        submit_add_locked(&low_prio_queue, work);
    } else {
        submit_add_locked(&high_prio_queue, work);
        system_softirq_trigger();
    }

    system_critical_section_exit();
}

void work_schedule_after(struct work *work, u32_ms_t delay)
{
    work_schedule_at(work, system_uptime_get_ms() + delay);
}

void work_schedule_again(struct work *work, u32_ms_t delay)
{
    work_schedule_at(work, work->scheduled_uptime + delay);
}

void work_schedule_at(struct work *work, u64_ms_t uptime)
{
    RUNTIME_ASSERT(work->priority >= 0);  // scheduling for high prio items not supported

    system_critical_section_enter();

    if (!test_flags_any(work, WORK_ITEM_SCHEDULED | WORK_ITEM_SUBMITTED)) {
        schedule_add_locked(work, uptime);
    }

    system_critical_section_exit();
}

void work_cancel(struct work *work)
{
    system_critical_section_enter();

    if (!test_flags_any(work, WORK_ITEM_SCHEDULED)) {
        remove_locked(&scheduled_queue, work, WORK_ITEM_SCHEDULED);
    }

    if (!test_flags_any(work, WORK_ITEM_SUBMITTED)) {
        remove_locked(&low_prio_queue, work, WORK_ITEM_SUBMITTED);
    }

    system_critical_section_exit();
}

/**
 * Submits all work items from the scheduled queue which are ready.
 */
static void submit_ready_work()
{
    u64_ms_t current_uptime = system_uptime_get_ms();

    system_critical_section_enter();

    struct work *work = scheduled_queue;

    // submit items
    while ((work != NULL) && (work->scheduled_uptime <= current_uptime)) {
        struct work *next = work->next;

        clear_flags(work, WORK_ITEM_SCHEDULED);
        work->next = NULL;

        submit_add_locked(&low_prio_queue, work);

        work = next;
    }

    // update queue head
    scheduled_queue = work;

    system_critical_section_exit();
}

/**
 * Processes the first queued item, if any.
 */
static bool_t process_next_work(struct work **queue)
{
    // remove first item from queue and update state
    system_critical_section_enter();

    struct work *work = *queue;

    if (work == NULL) {
        system_critical_section_exit();
        return false;
    }

    *queue = work->next;

    clear_flags(work, WORK_ITEM_SUBMITTED);
    set_flags(work, WORK_ITEM_RUNNING);
    work->next = NULL;

    system_critical_section_exit();

    // process item
    work->handler(work);

    // update state
    system_critical_section_enter();
    clear_flags(work, WORK_ITEM_RUNNING);
    system_critical_section_exit();

    return true;
}

/**
 * Enters sleep mode until the next scheduled work item becomes ready.
 */
static void sleep_until_ready()
{
    system_critical_section_enter();

    // don't go to sleep if there is still submitted work
    if (low_prio_queue != NULL) {
        system_critical_section_exit();
        return;
    }

    if (scheduled_queue != NULL) {
        u64_ms_t current_uptime = system_uptime_get_ms();

        // don't go to sleep if there is ready work
        if (scheduled_queue->scheduled_uptime < current_uptime) {
            system_critical_section_exit();
            return;
        }

        u64_ms_t wakeup_timeout = scheduled_queue->scheduled_uptime - current_uptime;

        // don't go to sleep if wake-up cannot be scheduled (timeout too short)
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
 * @param queue Queue to add the item to.
 * @param work Work item to add.
 */
static void submit_add_locked(struct work **queue, struct work *work)
{
    struct work *previous = NULL;
    struct work *next = *queue;

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
        *queue = work;
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
static void schedule_add_locked(struct work *work, u64_ms_t scheduled_uptime)
{
    struct work *previous = NULL;
    struct work *next = scheduled_queue;

    // find correct position to insert
    while (next != NULL) {
        if (next->scheduled_uptime > scheduled_uptime) {
            break;
        }

        previous = next;
        next = next->next;
    }

    // insert item
    if (previous != NULL) {
        previous->next = work;
    } else {
        scheduled_queue = work;
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
 * @param queue Queue to remove the item from.
 * @param work Work item to remove.
 * @param flags_to_clear Flags to clear on the work item if removed.
 */
static void remove_locked(struct work **queue, struct work *work, uint32_t flags_to_clear)
{
    struct work *previous = NULL;
    struct work *next = *queue;

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
            *queue = work->next;
        }

        clear_flags(work, flags_to_clear);
        work->next = NULL;
    }
}

/**
 * Helper function to set the specified flags on a work item.
 *
 * @param work Work item.
 * @param flags Flags to set.
 */
static void set_flags(struct work *work, uint32_t flags)
{
    work->flags |= flags;
}

/**
 * Helper function to clear the specified flags on a work item.
 *
 * @param work Work item.
 * @param flags Flags to set.
 */
static void clear_flags(struct work *work, uint32_t flags)
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
static bool_t test_flags_any(struct work *work, uint32_t flags)
{
    return (work->flags & flags) != 0;
}


#ifdef BUILD_UNIT_TEST

static void stop_request_handler(struct work *work)
{
    ARG_UNUSED(work);

    running = false;
}

static WORK_DEFINE(stop_request_work, INT32_MAX, stop_request_handler);

void work_run_for(u32_ms_t duration)
{
    work_schedule_after(&stop_request_work, duration);
    work_run();
}

#endif
