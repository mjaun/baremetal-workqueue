#include "util/unused.h"

extern void system_init(void);
extern void application_main(void);

int main(int argc, char *argv[])
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    system_init();
    application_main();
    return 0;
}
