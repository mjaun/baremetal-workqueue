#include "application/application_startup.h"
#include "application/peripherals_sim.h"
#include "service/system_sim.h"
#include "service/adapter_sim.h"
#include "util/unused.h"

int main(int argc, char *argv[])
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    system_setup();
    adapter_setup();
    peripherals_setup();

    application_main();
    return 0;
}
