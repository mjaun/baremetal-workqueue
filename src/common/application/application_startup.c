#include "application/application_startup.h"
#include "application/button_handler.h"
#include "application/uptime_printer.h"
#include "service/work.h"

void application_main(void)
{
    button_handler_init();
    uptime_printer_init();
    work_run();
}
