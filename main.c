#include "bus.h"
#include "rom.h"
#include "ram.h"
#include "cpu.h"

#define VERBOSE

#include <stdio.h>

extern uint8_t registers[];

void printBusRange(const uint16_t start, const uint16_t stop) {
	if (start > stop) {
		printBusRange(stop, start);
		return;
	}
	for (size_t i = start & 0xFFF0; i <= stop; i += 0x0010) {
		if (i % 0x0100 == 0)
			printf("\n");

		printf("$%04X: ", (uint16_t) i);
		for (uint8_t j = 0; j < 0x10; j++) {
			if ((i + j) < start || (i + j) > stop)
				printf("__ ");
			else
				printf("%02X ", bus_read((uint16_t) i + j));
			if (j == 7)
				printf("\t");
		}
		printf("\n");
	}
}

static inline void printStackPage() {
	printf("SP: %02X", registers[6]); // stack pointer
	printBusRange(0x0100, 0x01FF);
}

int main() {
	bus_init();

	component_t ram = ram_init(0x10000);
	component_t rom = rom_init(0x10000);
	bus_add(&ram, 0x0000, 0xFFFF);
	ram_randomize(&ram);
	rom_loadFile(&rom, "test_6502.bin", 0x000a);
	ram_loadFile(&ram, "test_6502.bin", 0x000a);

	cpu_reset();
	uint16_t* programCounter = (uint16_t*) registers;
	*programCounter = 0x0400;

	printf("running:\n");
	// run x amount of instructions before going more in depth
	for (size_t i = 0; i < 26765875; i++)
		cpu_runInstruction();

	// stops program execution when there was a jump/branch to the exact same position
	// this is how the test program indicates an incorrect instruction
	uint16_t prevProgramCounter = 0;
	extern bool ranUnimplementedInstruction;
	while (*programCounter != prevProgramCounter && !ranUnimplementedInstruction) {
		prevProgramCounter = *programCounter;
		
#ifdef VERBOSE
		cpu_printOpcode();
#endif

		cpu_runInstruction();

#ifdef VERBOSE
		cpu_printRegisters();
		printf("\n");
#endif
	}
	for (int i = 0; i < 0; i++) {
		prevProgramCounter = *programCounter;

#ifdef VERBOSE
		cpu_printOpcode();
#endif

		cpu_runInstruction();

#ifdef VERBOSE
		//cpu_printRegisters();
		//printf("\n");
#endif
	}

	extern size_t instructionCount;
	extern size_t totalCycles;
	printf("ended at $%04X\n", prevProgramCounter);
	printf("test number: %d\n", bus_read(0x0200));
	printf("ran %zi instruction(s) in %zi clockcycle(s)\n", instructionCount, totalCycles);

	rom_destroy(rom);
	ram_destroy(ram);
	bus_destroy();

	return 0;
}
