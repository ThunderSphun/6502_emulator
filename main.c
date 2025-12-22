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
	printf("%04X - %04X\n", from, to); \
	/*printf("   %4s - %4s   \t(%c%c)\treturned ", &#from[2], &#to[2], read ? 'R' : '_', write ? 'W' : '_');*/ \
	if (bus_add(read, write, from, to)) { \
		/*printf("true\n");*/ \
		bus_print(); \
		/*printf("\n\n");*/ \
	} else {\
		/*printf("false\n");*/ \
	} \
	bus_destroy()

int main() {
	// bus_init();
	// printf("- initial state\n");
	// bus_print();
	// bus_destroy();
	// printf("\n\n");

	// break each edge (one edge per edit)
	TEST(0x0000, 0x01FF, NULL, NULL); // 0x0000 - 0x01FF, 0x0200 - 0xFEFF, 0xFF00 - 0xFFFF
	TEST(0x0000, 0x7FFF, NULL, NULL); // 0x0000 - 0x7FFF, 0x8000 - 0xFEFF, 0xFF00 - 0xFFFF
	TEST(0x0100, 0xFEFF, NULL, NULL); // 0x0000 - 0x00FF, 0x0100 - 0xFEFF, 0xFF00 - 0xFFFF
	TEST(0x8000, 0xFFFF, NULL, NULL); // 0x0000 - 0x00FF, 0x0100 - 0x7FFF, 0x8000 - 0xFFFF
	TEST(0xFE00, 0xFFFF, NULL, NULL); // 0x0000 - 0x00FF, 0x0100 - 0xFDFF, 0xFE00 - 0xFFFF

	// break each edge at once
	TEST(0x0080, 0xFF7F, NULL, NULL); // 0x0000 - 0x007F, 0x0080 - 0xFF7F, 0xFF80 - 0xFFFF
	TEST(0x0000, 0xFFFF, NULL, NULL); // 0x0000 - 0xFFFF

	return 0;
}