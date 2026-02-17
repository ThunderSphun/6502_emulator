#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

void cpu_irq(bool active);
void cpu_reset(bool active);
void cpu_nmi(bool active);

void cpu_clock();
void cpu_runInstruction();

void cpu_printRegisters();
void cpu_printOpcode();
