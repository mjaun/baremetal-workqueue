#include "util/unused.h"

extern void application_main(void);

int main(int argc, char *argv[])
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    application_main();
    return 0;
}
