extern void button_handler_init(void);
extern void uptime_printer_init(void);

void application_main(void)
{
    button_handler_init();
    uptime_printer_init();
}
