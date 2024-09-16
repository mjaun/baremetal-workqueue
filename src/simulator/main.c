#include "application/application_startup.h"
#include "service/system_sim.h"
#include "util/unused.h"

int main(int argc, char *argv[])
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    system_init();
    application_main();
    return 0;
}
