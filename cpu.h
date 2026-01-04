#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

void cpu_irq();
void cpu_reset();
void cpu_nmi();

void cpu_clock();
void cpu_runInstruction();
