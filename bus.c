#include "bus.h"

#include <stdlib.h>

bool bus_init(bus_t* bus) {
	bus->addresses = malloc(sizeof(busAddr_t));
	if (bus->addresses == NULL)
		return false;
	bus->size = 1;

	bus->addresses[0].begin = 0x0000;
	bus->addresses[0].end = 0xFFFF;
	bus->addresses[0].read = NULL;
	bus->addresses[0].write = NULL;

	return true;
}

bool bus_destroy(bus_t* bus) {
	free(bus->addresses);
	bus->size = 0;

	return true;
}

void bus_print(bus_t* bus) {
	for (size_t i = 0; i < bus->size; i++) {
		busAddr_t* current = bus->addresses + i;

		if (i != 0)
			printf("\n");

		printf("%016b\t%04X\n", current->begin, current->begin);
		if (current->begin != current->end) {
			printf("................\t....\n");
			printf("%016b\t%04X\n", current->end, current->end);
		}
	}
}
