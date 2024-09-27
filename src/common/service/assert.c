#include "service/assert.h"
#include "service/system.h"
#include "service/log.h"

LOG_MODULE_REGISTER(assert);

void __assert_handler(const char *file, unsigned line)
{
    static bool in_assert = false;

    // an assert in the logging framework for example may trigger recursively
    if (in_assert) {
        return;
    }

    in_assert = true;

    LOG_ERR("%s:%u", file, line);

    system_fatal_error();
}
