#include "service/work.h"
#include "service/log.h"

LOG_MODULE_REGISTER(uptime_printer);

static void print_uptime(struct work *work);

static WORK_DEFINE(work_uptime, 1, print_uptime);

void uptime_printer_init(void)
{
    work_submit(&work_uptime);
}

static void print_uptime(struct work *work)
{
    LOG_INF("print uptime");
    work_schedule_again(work, 1000);
}
