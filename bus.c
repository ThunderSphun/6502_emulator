#include "bus.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool bus_init(bus_t* bus) {
	bus->addresses = malloc(sizeof(busAddr_t) * 4);
	if (bus->addresses == NULL)
		return false;
	bus->size = 4;

	bus->addresses[0].begin = 0x0000;
	bus->addresses[0].end = 0x3FFF;
	bus->addresses[0].read = NULL;
	bus->addresses[0].write = NULL;

	bus->addresses[1].begin = 0x4000;
	bus->addresses[1].end = 0x7FFF;
	bus->addresses[1].read = NULL;
	bus->addresses[1].write = NULL;

	bus->addresses[2].begin = 0x8000;
	bus->addresses[2].end = 0xBFFF;
	bus->addresses[2].read = NULL;
	bus->addresses[2].write = NULL;

	bus->addresses[3].begin = 0xC000;
	bus->addresses[3].end = 0xFFFF;
	bus->addresses[3].read = NULL;
	bus->addresses[3].write = NULL;

	return true;
}

bool bus_add(bus_t* bus, busReadFunc readFunc, busWriteFunc writeFunc, uint16_t begin, uint16_t end) {
	if (bus->addresses == NULL)
		return false;

	for (size_t i = 0; i < bus->size; i++) {
		busAddr_t* current = bus->addresses + i;

		if (current->begin == begin && current->end == end) {
			current->read = readFunc;
			current->write = writeFunc;

			return true;
		}
		if (current->begin == begin && current->end > end) {
			{
				busAddr_t* nextList = realloc(bus->addresses, sizeof(busAddr_t) * (bus->size + 1));
				if (nextList == NULL)
					return false;
				bus->addresses = nextList;
			}

			current = bus->addresses + i; // reset current ptr because data might have moved
			memmove(current + 1, current, sizeof(busAddr_t) * (bus->size - i));

			current->begin = begin;
			current->end = end;
			current->read = readFunc;
			current->write = writeFunc;
			(current + 1)->begin = end + 1;

			bus->size++;

			return true;
		}
	}

	return false;
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
			printf("................\t....\tr%p w%p\n", current->read, current->write);
			printf("%016b\t%04X\n", current->end, current->end);
		}
	}
}
