#include "bus.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

uint8_t read(uint16_t addr) {
	return 0;
}
void write(uint16_t addr, uint8_t data) {
}

#define TEST(from, to, read, write) \
	bus_init(); \
	printf("   %4s - %4s   \t(%c%c)\treturned ", #from + 2, #to + 2, read ? 'R' : '_', write ? 'W' : '_'); \
	if (bus_add(read, write, from, to)) { \
		printf("true\n"); \
		bus_print(); \
		printf("\n\n"); \
	} else \
		printf("false\n"); \
	bus_destroy()

int main() {
	bus_init();
	printf("- initial state\n");
	bus_print();
	bus_destroy();
	printf("\n\n");

	printf("whole block\n\n");
	TEST(0x0000, 0x00FF, read, write);
	TEST(0x0100, 0xFEFF, read, write);
	TEST(0xFF00, 0xFFFF, read, write);

	printf("front edge of block\n\n");
	TEST(0x0000, 0x007F, read, write);
	TEST(0x0100, 0x7FFF, read, write);
	TEST(0xFF00, 0xFF7F, read, write);

	printf("back edge of block\n\n");
	TEST(0x0080, 0x00FF, read, write);
	TEST(0x8000, 0xFEFF, read, write);
	TEST(0xFF80, 0xFFFF, read, write);

	printf("middle of block\n\n");
	TEST(0x0040, 0x00BF, read, write);
	TEST(0x4000, 0xBFFF, read, write);
	TEST(0xFF40, 0xFFBF, read, write);

	return 0;
}