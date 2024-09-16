#include "application/button_handler.h"
#include "application/uptime_printer.h"

void application_main(void)
{
    button_handler_init();
    uptime_printer_init();
}
