#include "bus.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct bus bus_t;
typedef struct busAddr busAddr_t;

struct bus {
	busAddr_t* addresses;
	size_t size;
	bool initialized;
};

struct busAddr {
	uint16_t start;
	uint16_t stop;
	busReadFunc read;
	busWriteFunc write;
};

bus_t bus = { 0 };

bool bus_init() {
	if (bus.initialized)
		return false;

	bus.addresses = malloc(sizeof(busAddr_t) * 3);
	if (bus.addresses == NULL)
		return false;
	bus.size = 3;

	bus.addresses[0].start = 0x0000;
	bus.addresses[0].stop = 0x00FF;
	bus.addresses[0].read = NULL;
	bus.addresses[0].write = NULL;

	bus.addresses[1].start = 0x0100;
	bus.addresses[1].stop = 0xFEFF;
	bus.addresses[1].read = NULL;
	bus.addresses[1].write = NULL;

	bus.addresses[2].start = 0xFF00;
	bus.addresses[2].stop = 0xFFFF;
	bus.addresses[2].read = NULL;
	bus.addresses[2].write = NULL;

	bus.initialized = true;
	return true;
}

bool bus_add(busReadFunc readFunc, busWriteFunc writeFunc, uint16_t start, uint16_t stop) {
	if (!bus.initialized)
		return false;

	if (start > stop) {
		uint16_t tmp = start;
		start = stop;
		stop = tmp;
	}

	busAddr_t* startAddr = NULL;
	busAddr_t* stopAddr = NULL;

	for (size_t i = 0; i < bus.size; i++) {
		busAddr_t* current = bus.addresses + i;

		if (start >= current->start && start <= current->stop)
			startAddr = current;

		if (stop >= current->start && stop <= current->stop)
			stopAddr = current;
	}

	if (startAddr == NULL || stopAddr == NULL)
		return false;	// shouldn't be possible, bus should always at least have a range of 0x0000-0xFFFF
						// keep this for a sanity check

	if (startAddr == stopAddr) {
		// early return for simple change
		if (startAddr->start == start && startAddr->stop == stop) {
			startAddr->read = readFunc;
			startAddr->write = writeFunc;

			return true;
		}

		size_t index = startAddr - bus.addresses;
		bool addBefore = start != startAddr->start;
		bool addAfter = stop != startAddr->stop;

		// allocate new memory
		{
			busAddr_t* nextList = realloc(bus.addresses, sizeof(busAddr_t) * (bus.size + addBefore + addAfter));
			if (nextList == NULL)
				return false;

			bus.addresses = nextList;
		}

		startAddr = bus.addresses + index + addBefore; // reset startAddr ptr because data might have moved
		memmove(startAddr + addAfter, startAddr - addBefore, sizeof(busAddr_t) * (bus.size - index));

		// update altered bus ranges
		startAddr->start = start;
		startAddr->stop = stop;
		startAddr->read = readFunc;
		startAddr->write = writeFunc;
		if (addBefore)
			(startAddr - 1)->stop = start - 1;
		if (addAfter)
			(startAddr + 1)->start = stop + 1;

		bus.size += addBefore + addAfter;

		return true;
	}

	return false;
}

bool bus_destroy() {
	if (!bus.initialized)
		return false;

	free(bus.addresses);
	bus.initialized = false;

	return true;
}

#ifdef __GNUC__
void printBin(uint16_t val) {
	printf("%08b %08b", (val >> 8) & 0xFF, val & 0xFF);
}
#else
void printBin(uint16_t val) {
	for (int i = 15; i >= 0; i--) {
		printf("%c", (val & (1 << i)) ? '1' : '0');
		if (i == 8)
			printf(" ");
	}
}
#endif

void bus_print() {
	if (!bus.initialized) {
		printf("bus not initialized");
		return;
	}

	for (size_t i = 0; i < bus.size; i++) {
		busAddr_t* current = bus.addresses + i;

		if (i != 0)
			printf("\n");

		printBin(current->start);
		printf("\t%04X", current->start);
		if (current->start != current->stop) {
			printf("\n");
			printf("........ ........\t....\tr%p w%p\n", current->read, current->write);
			printBin(current->stop);
			printf("\t%04X\n", current->stop);
		} else
			printf("\tr%p w%p\n", current->read, current->write);
	}
}
