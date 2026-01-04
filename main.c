#include "bus.h"
#include "rom.h"
#include "ram.h"
#include "cpu.h"

#include <stdio.h>

extern struct registers {
	uint16_t PC;
} registers;

extern size_t cycles;

void printBusRange() {
	for (size_t i = 0; i < 0x10000; i += 0x0010) {
		if (i % 0x0100 == 0)
			printf("\n");

		printf("$%04X: ", (uint16_t) i);
		for (uint8_t j = 0; j < 0x10; j++) {
			printf("%02X ", bus_read((uint16_t) i + j));
			if (j == 7)
				printf("\t");
		}
		printf("\n");
	}
	printf("\n");
}

int main() {
	bus_init();

	component_t ram = ram_init(0x8000);
	component_t rom = rom_init(0x8000);

	rom_set(&rom, 0x7FFC, 2, (uint8_t[]) { 0x00, 0x80 });

	bus_add(&ram, 0x0000, 0x7FFF);
	bus_add(&rom, 0x8000, 0xFFFF);

	printf("cpu_clock:\n");
	cpu_reset();
	for (int i = 0; i < 20; i++) {
		printf("%04X, %zi\n", registers.PC, cycles);
		cpu_clock();
	}

	printf("cpu_runInstruction:\n");
	cpu_reset();
	for (int i = 0; i < 5; i++) {
		printf("%04X, %zi\n", registers.PC, cycles);
		cpu_runInstruction();
	}

	rom_destroy(rom);
	ram_destroy(ram);
	bus_destroy();

	return 0;
}
