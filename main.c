#include "bus.h"
#include "rom.h"
#include "ram.h"
#include "cpu.h"

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
	printf("\n");
}

void printStackPage() {
	printf("%02X", registers[6]); // stack pointer
	printBusRange(0x0100, 0x01FF);
}

int main() {
	bus_init();

	component_t ram = ram_init(0x10000);
	component_t rom = rom_init(0x10000);
	bus_add(&ram, 0x0000, 0xFFFF);
	ram_randomize(&ram);
	rom_loadFile(&rom, "test_functional.bin", 0x000a);
	ram_loadFile(&ram, "test_functional.bin", 0x000a);

	cpu_reset();
	uint16_t* programCounter = (uint16_t*) registers;
	*programCounter = 0x0400;

	printf("running:\n");
	// stops program execution when there was a jump/branch to the exact same position
	// this is how the test program indicates an incorrect instruction
	uint16_t prevProgramCounter = 0;
	while (*programCounter != prevProgramCounter) {
	//for (int i = 0; i < 20; i++) {
		prevProgramCounter = *programCounter;

		cpu_runInstruction();
		cpu_printRegisters();
		printf("\n");
	}

	printf("ended at $%04X\n", prevProgramCounter);
	printf("test number: #%02X\n", bus_read(0x0200));

	rom_destroy(rom);
	ram_destroy(ram);
	bus_destroy();

	return 0;
}
