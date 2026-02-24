#pragma once

#include <stdbool.h>

/// emulates pins from 6502, need to be high for at least one clock pulse to be detected
/// see cpu_clock for more info
void cpu_irq(const bool active);
void cpu_reset(const bool active);
void cpu_nmi(const bool active);

/// performs a single clock cycle
/// internally cycles are consumed if there are cycles left to consume
/// when there are no cycles left to consume, the control inputs are checked
/// if no action is needed the next instruction is run instantly
/// else the correct action for the control input is performed
/// setting the amount of cycles to consume
/// on western design center chips there are ways to halt the processor
/// if the chip is halted, cpu_clock checks if it can continue, based on the state of the control inputs
void cpu_clock();

/// performs a single instruction
/// first makes sure no more cycles are needed to be consumed
/// then runs the actual instruction and consumes all its cycles
/// a call to cpu_clock after cpu_runInstruction will instantly perform the next instructions
/// on western design center chips there are ways to halt the processor
/// if the chip is halted, cpu_runInstruction checks if it can continue, and performs one instruction if so. else it will do nothing
void cpu_runInstruction();

void cpu_printRegisters();
void cpu_printOpcode();
