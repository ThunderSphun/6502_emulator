#include "bus.h"
#include "rom.h"
#include "ram.h"
#include "cpu.h"

#include <stdio.h>

extern struct registers {
	uint16_t PC;
	uint8_t A;
	uint8_t X;
	uint8_t Y;
	uint8_t P;
	uint8_t SP;
} registers;

extern size_t cycles;

void printBusRange(const uint16_t start, const uint16_t stop) {
	if (start > stop) {
		printBusRange(stop, start);
		return;
	}
	for (size_t i = start; i <= stop; i += 0x0010) {
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

void printStackPage() {
	printf("%02X", registers.SP);
	printBusRange(0x0100, 0x01FF);
}

int main() {
	bus_init();

	component_t ram = ram_init(0x8000);
	component_t rom = rom_init(0x8000);

	ram_randomize(&ram);
	rom_set(&rom, 0x7FFA, 6, (uint8_t[]) { 0x00, 0xA0, 0x00, 0x80, 0x00, 0xF0 });
	rom_set(&rom, 0, 1, (uint8_t[]) { 0x00 });

	bus_add(&ram, 0x0000, 0x7FFF);
	bus_add(&rom, 0x8000, 0xFFFF);

	cpu_reset();
	cpu_runInstruction();
	cpu_runInstruction();
	cpu_runInstruction();

	rom_destroy(rom);
	ram_destroy(ram);
	bus_destroy();

	return 0;
}
