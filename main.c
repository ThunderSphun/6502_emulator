#include "bus.h"
#include "rom.h"
#include "ram.h"

#include <stdio.h>

void printBusRange() {
	for (size_t i = 0; i < 0x10000; i += 0x0010) {
		if (i % 0x0100 == 0)
			printf("\n");

		printf("$%04X: ", (uint16_t) i);
		for (uint8_t j = 0; j < 0x10; j++) {
			printf("%02X ", busRead((uint16_t) i + j));
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

	bus_add(&ram, 0x0000, 0x7FFF);
	bus_add(&rom, 0x8000, 0xFFFF);
	bus_print();

	printBusRange();
	for (size_t i = 0; i < 0x10000; i++)
		//busWrite((uint16_t) i, (uint8_t) i);
		busWrite((uint16_t) i, (uint8_t) ((rand() / (float) RAND_MAX) * UINT8_MAX));
	printBusRange();

#define TEST(addr) \
	printf("bus read at $%04X: %i\n", addr, busRead(addr));\
	busWrite(addr, 41);\
	printf("bus read at $%04X: %i\n\n", addr, busRead(addr))

	TEST(0x0000);
	TEST(0x1000);
	TEST(0x7000);
	TEST(0x7FFF);
	TEST(0x8000);
	TEST(0x9000);
	TEST(0xF000);
	TEST(0xFFFF);

#undef TEST

	printBusRange();

	bus_print();

	rom_destroy(rom);
	ram_destroy(ram);
	bus_destroy();

	return 0;
}
