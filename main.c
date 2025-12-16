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

int main() {
	bus_init();
	bus_print();
	printf("-------------------------------------\n");
	bus_add(read, write, 0x8000, 0xBFFF);
	bus_print();
	printf("-------------------------------------\n");
	bus_add(NULL, write, 0x0000, 0x0000);
	bus_print();
	printf("-------------------------------------\n");
	bus_add(NULL, NULL, 0x4000, 0x4FFF);
	bus_print();
	printf("-------------------------------------\n");
	bus_add(NULL, NULL, 0x7000, 0x7FFF);
	bus_print();
	bus_destroy();
	return 0;
}