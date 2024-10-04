#include <service/work.h>
#include <service/system.h>
#include <service/assert.h>
#include <util/unused.h>

static volatile bool_t running = false;
static struct work *high_prio_queue = NULL;
static struct work *low_prio_queue = NULL;
static struct work *scheduled_queue = NULL;

static bool_t process_next_work(struct work **queue);
static void schedule_timer_locked();
static void idle_sleep();

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

    // process low prio queue
    while (running) {
        if (!process_next_work(&low_prio_queue)) {
            idle_sleep();
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
    system_critical_section_enter();

    if (!test_flags_any(work, WORK_ITEM_SCHEDULED | WORK_ITEM_SUBMITTED)) {
        schedule_add_locked(work, uptime);
        schedule_timer_locked();
    }

    system_critical_section_exit();
}

void work_cancel(struct work *work)
{
    system_critical_section_enter();

    if (test_flags_any(work, WORK_ITEM_SCHEDULED)) {
        remove_locked(&scheduled_queue, work, WORK_ITEM_SCHEDULED);
    }

    if (test_flags_any(work, WORK_ITEM_SUBMITTED)) {
        if (work->priority >= 0) {
            remove_locked(&low_prio_queue, work, WORK_ITEM_SUBMITTED);
        } else {
            remove_locked(&high_prio_queue, work, WORK_ITEM_SUBMITTED);
        }
    }

    system_critical_section_exit();
}

/**
 * Schedules the system timer based on the first item in the scheduled queue, if any.
 *
 * Interrupts must be locked.
 */
static void schedule_timer_locked()
{
    struct work* next = scheduled_queue;

    if (next != NULL) {
        system_timer_schedule_at(next->scheduled_uptime);
    }
}

/**
 * Processes the first queued item, if any.
 *
 * @return True if an item was processed, false if there was none.
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
 * Enters sleep mode if there is no more work submitted.
 */
static void idle_sleep()
{
    system_critical_section_enter();

    // don't go to sleep if there is still work submitted
    if (low_prio_queue != NULL) {
        system_critical_section_exit();
        return;
    }

    system_enter_sleep_mode();

    system_critical_section_exit();
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

void system_timer_handler(void)
{
    system_critical_section_enter();

    u64_ms_t current_uptime = system_uptime_get_ms();
    struct work *work = scheduled_queue;

    // submit ready items
    while ((work != NULL) && (work->scheduled_uptime <= current_uptime)) {
        struct work *next = work->next;

        clear_flags(work, WORK_ITEM_SCHEDULED);
        work->next = NULL;

        if (work->priority >= 0) {
            submit_add_locked(&low_prio_queue, work);
        } else {
            submit_add_locked(&high_prio_queue, work);
            system_softirq_trigger();
        }

        work = next;
    }

    // update queue head
    scheduled_queue = work;

    // schedule next timer interrupt
    schedule_timer_locked();

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
