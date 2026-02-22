#include "bus.h"
#include "memory.h"
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

uint8_t irqData = 0;

uint8_t irqTest_readFunc(const device_t* const device, const addr_t addr) {
	(void) addr;
	return *(uint8_t*) device->device_data;
}

void irqTest_writeFunc(const device_t* const device, const addr_t addr, const uint8_t data) {
	(void) addr;
	cpu_irq(data & (1 << 0));
	cpu_nmi(data & (1 << 1));
	*(uint8_t*)device->device_data = data;
}

void handleInput() {
	extern uint8_t signals;
	while (signals & 0xC0) {
		printf("r: cpu_reset toggle\ni: cpu_irq toggle\nn: cpu_nmi toggle\nC: cpu_clock single\nI: cpu_runInstruction once\nEnter option: ");
		char c = getchar();
		switch (c) {
		case 'r': { cpu_reset(!(signals & 0x02)); break; }
		case 'i': { cpu_irq  (!(signals & 0x01)); break; }
		case 'n': { cpu_nmi  (!(signals & 0x04)); break; }

		case 'C': { cpu_clock(); break; }
		case 'I': { cpu_runInstruction(); break; }
		}

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
}

uint8_t testRead(const device_t* const device, const addr_t addr) {
	(void) device;
	(void) addr;
	printf("testRead\n");
	return 0xff;
}

void testWrite(const device_t* const device, const addr_t addr, const uint8_t data) {
	(void) device;
	(void) addr;
	printf("testWrite %02X\n", data);
}

int main() {
	bus_init();
	bus_print();
	printf("\n");

	device_t devices[] = {
		{ .name="device 0", .readFunc = NULL,     .writeFunc = testWrite },
		{ .name="device 1", .readFunc = testRead, .writeFunc = testWrite },
		{ .name="device 2", .readFunc = testRead, .writeFunc = NULL      },
		{ .name="device 3", .readFunc = testRead, .writeFunc = testWrite },
		{ .name="device 4", .readFunc = testRead, .writeFunc = testWrite },
		{ .name="device 5", .readFunc = testRead, .writeFunc = testWrite },
	};

	bus_read(0x0000);
	bus_read(0xffff);
	bus_read(0x8000);
	bus_write(0x0000, 0xaa);
	bus_write(0xffff, 0xaa);
	bus_write(0x8000, 0xaa);

	bus_add(devices + 0, 0x4000, 0xffff);
	bus_print();
	printf("\n");

	bus_read(0x0000);
	bus_read(0xffff);
	bus_read(0x8000);
	bus_write(0x0000, 0xaa);
	bus_write(0xffff, 0xaa);
	bus_write(0x8000, 0xaa);

	bus_add(devices + 1, 0x7000, 0x8fff);
	bus_print();
	printf("\n");

	bus_read(0x0000);
	bus_read(0xffff);
	bus_read(0x8000);
	bus_write(0x0000, 0xaa);
	bus_write(0xffff, 0xaa);
	bus_write(0x8000, 0xaa);

	bus_add(devices + 2, 0x0000, 0x000f);
	bus_print();
	printf("\n");

	bus_read(0x0000);
	bus_read(0xffff);
	bus_read(0x8000);
	bus_write(0x0000, 0xaa);
	bus_write(0xffff, 0xaa);
	bus_write(0x8000, 0xaa);

	bus_destroy();

	return 0;

	device_t ram = memory_init(0x10000, true);
	device_t rom = memory_init(0x10000, false);

	device_t irqTest = (device_t){ &irqData, "irq test", irqTest_readFunc, irqTest_writeFunc};

	if (!bus_add(&ram, 0x0000, 0xFFFF)) {
		memory_destroy(rom);
		memory_destroy(ram);
		bus_destroy();
		return -1;
	}
	if (!bus_add(&irqTest, 0xbffc, 0xbffc)) {
		memory_destroy(rom);
		memory_destroy(ram);
		bus_destroy();
		return -1;
	}
	if (!memory_randomize(&ram)) {
		memory_destroy(rom);
		memory_destroy(ram);
		bus_destroy();
		return -1;
	}
	const char* binFile = "test_6502.bin";
	printf("%s\n", binFile);
	if (!memory_loadFile(&rom, binFile, 0x000a)) {
		memory_destroy(rom);
		memory_destroy(ram);
		bus_destroy();
		return -1;
	}
	if (!memory_loadFile(&ram, binFile, 0x000a)) {
		memory_destroy(rom);
		memory_destroy(ram);
		bus_destroy();
		return -1;
	}

	cpu_reset(true);
	cpu_clock();
	cpu_reset(false);
	uint16_t* programCounter = (uint16_t*) registers;
	*programCounter = 0x0400;

	printf("running:\n");

	// stops program execution when there was a jump/branch to the exact same position
	// this is how the test program indicates an incorrect instruction
	uint16_t prevProgramCounter = 0;
	extern bool ranUnimplementedInstruction;
	extern uint8_t signals;
	while (*programCounter != prevProgramCounter && !ranUnimplementedInstruction) {
		prevProgramCounter = *programCounter;

#ifdef VERBOSE
		cpu_printOpcode();
#endif

		cpu_runInstruction();

		if (signals & 0xC0) {
#ifdef VERBOSE
			printf("WAI is%sset, STP is %s set\n", signals & 0x40 ? " " : " not ", signals & 0x80 ? " " : " not ");
#endif
			handleInput();
		}

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

	memory_destroy(rom);
	memory_destroy(ram);
	bus_destroy();

	return 0;
}
