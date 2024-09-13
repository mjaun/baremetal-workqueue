#include "drivers/sleep.h"
#include <unistd.h>

void msleep(u32_ms_t timeout)
{
    usleep(timeout * 1000);
}
