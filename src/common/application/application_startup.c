#include "application/application_startup.h"
#include "application/button_handler.h"
#include "application/uptime_printer.h"
#include "service/work.h"
#include "service/log.h"

LOG_MODULE_REGISTER(application_startup);

void application_main(void)
{
    log_set_level("application_statup", LOG_LEVEL_WRN);

    LOG_ERR("this is an error: %d", LOG_LEVEL_ERR);
    LOG_WRN("this is a warning: %d", LOG_LEVEL_WRN);
    LOG_INF("this is an info: %d", LOG_LEVEL_INF);
    LOG_DBG("this is a debug message: %d", LOG_LEVEL_DBG);

    button_handler_init();
    uptime_printer_init();
    work_run();
}
