#pragma once

#include "util/types.h"

void system_critical_section_enter();
void system_critical_section_exit();

void system_msleep(u32_ms_t timeout);
u64_ms_t system_uptime_get();
