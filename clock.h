#pragma once

#include <stdint.h>
#include <stdbool.h>

extern bool clock_running;

void clock_reset();
void clock_run(uint64_t targetFrequency);