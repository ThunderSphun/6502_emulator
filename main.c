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
	bus_t bus;
	bus_init(&bus);
	bus_print(&bus);
	printf("\n-------------------------------------\n");
	bus_add(&bus, read, write, 0x8000, 0xBFFF);
	bus_print(&bus);
	printf("\n-------------------------------------\n");
	bus_add(&bus, NULL, NULL, 0x4000, 0x4FFF);
	bus_print(&bus);
	printf("%p\n", bus.addresses);
	bus_destroy(&bus);
	return 0;
}