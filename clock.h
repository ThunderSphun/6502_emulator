#pragma once

#include <stddef.h>
#include <stdbool.h>

extern bool clock_running;

void clock_reset();
void clock_run(long targetFrequency);