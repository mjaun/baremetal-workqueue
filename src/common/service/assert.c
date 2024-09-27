#include "service/assert.h"
#include "service/system.h"
#include "service/log.h"

LOG_MODULE_REGISTER(assert);

void __assert_handler(const char *file, unsigned line)
{
    LOG_ERR("%s:%u", file, line);
    system_fatal_error();
}
