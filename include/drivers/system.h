#pragma once

#include "util/types.h"

void system_critical_section_enter();
void system_critical_section_exit();

bool_t system_schedule_wakeup(u64_us_t timeout);
void system_enter_sleep_mode();

u64_us_t system_uptime_get();
