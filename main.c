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
	bus_print();

	component_t ram = ram_init(0x8000);
	component_t rom = rom_init(0x8000);

	bus_add(&ram, 0x0000, 0x7FFF);
	bus_add(&rom, 0x8000, 0xFFFF);

	printf("%s\n", rom_set(&rom, 0x0000, 0x0010, (uint8_t[]) { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }) ? "true" : "false");

	for (uint8_t j = 0; j < 0x10; j++)
		printf("%02X ", bus_read((uint16_t) 0x8000 + j));
	printf("\n");

	printf("%s\n", rom_set(&rom, 0x8000 - 6, 6, (uint8_t[]) { 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF }) ? "true" : "false");

	for (int8_t j = 5; j >= 0; j--)
		printf("%02X ", bus_read((uint16_t) 0xFFFF - j));
	printf("\n");

	printf("%s\n", ram_set(&ram, 0x0000, 0x0010, (uint8_t[]) { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }) ? "true" : "false");

	for (uint8_t j = 0; j < 0x10; j++)
		printf("%02X ", bus_read((uint16_t) 0x0000 + j));
	printf("\n");

	printf("%s\n", ram_set(&ram, 0x8000 - 6, 6, (uint8_t[]) { 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF }) ? "true" : "false");

	for (int8_t j = 5; j >= 0; j--)
		printf("%02X ", bus_read((uint16_t) 0x7FFF - j));
	printf("\n");

	bus_print();

	bus_print();

	rom_destroy(rom);
	ram_destroy(ram);
	bus_destroy();

	return 0;
}
