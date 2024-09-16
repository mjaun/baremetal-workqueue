#include "service/work_ext.h"
#include "util/unused.h"

static void exit_request_handler(struct work *work);

static WORK_DEFINE(exit_request_work, UINT32_MAX, exit_request_handler);

void work_run_for(u32_ms_t timeout)
{
    work_schedule_after(&exit_request_work, timeout);
    work_run();
}

static void exit_request_handler(struct work *work)
{
    ARG_UNUSED(work);
    work_exit_request();
}
