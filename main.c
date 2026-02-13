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
		if (i % 0x0100 == 0 && i != start)
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
	printf("SP: %02X\n", registers[6]); // stack pointer
	printBusRange(0x0100, 0x01FF);
}

uint8_t irqTest_readFunc(const component_t* const component, const addr_t addr) {
	(void) addr;
	return *(uint8_t*) component->component_data;
}

void irqTest_writeFunc(const component_t* const component, const addr_t addr, const uint8_t data) {
	(void) addr;
	if (data & (1 << 0))
		cpu_irq();
	if (data & (1 << 1))
		cpu_nmi();
	*(uint8_t*)component->component_data = data;
}

int main() {
	bus_init();

	component_t ram = ram_init(0x10000);
	component_t rom = rom_init(0x10000);

	uint8_t irqData = 0;

	component_t irqTest = (component_t){ &irqData, "irq test", irqTest_readFunc, irqTest_writeFunc};

	bus_add(&ram, 0x0000, 0xFFFF);
	bus_add(&irqTest, 0xbffc, 0xbffc);
	ram_randomize(&ram);
	const char* binFile = "test_interupt.bin";
	printf("%s\n", binFile);
	rom_loadFile(&rom, binFile, 0x000a);
	ram_loadFile(&ram, binFile, 0x000a);

	cpu_reset();
	uint16_t* programCounter = (uint16_t*) registers;
	*programCounter = 0x0400;

	printf("running:\n");

	for (int i = 0; i < 222; i++)
		cpu_runInstruction();

	for (int i = 0x0100; i <= 0x01FF; i++)
		bus_write(i, 0);

	bus_write(0x000A, 0);
	bus_write(0x000B, 0);
	bus_write(0x000C, 0);

	bus_write(0x0203, 0);

	system("cls");

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
		printf(
			"irq_a  ($000A): %02X\nirq_x  ($000B): %02X\nirq_f  ($000C): %02X\n"
			"I_src  ($0203): %02X\nI_port ($BFFC): %02X\n",
			bus_read(0x000A), bus_read(0x000B), bus_read(0x000C),
			bus_read(0x0203), irqData);
		printf("SP: %02X ", registers[6]);
		for (int i = 0xF0; i <= 0xFF; i++) {
			if (i == registers[6])
				printf("vv ");
			else
				printf("   ");

			if (i == 0xF7)
				printf(" ");
		}
		printf("\n");
		printBusRange(0x01F0, 0x01FF);

		cpu_printRegisters();
		printf("\n");
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
