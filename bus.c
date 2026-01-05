#include "bus.h"

#include <stdio.h>

typedef struct busAddr busAddr_t;

static struct bus {
	busAddr_t* addresses;
	size_t size;
	bool initialized;
} bus = { 0 };

struct busAddr {
	uint16_t start;
	uint16_t stop;
	const component_t* component;
};

bool bus_init() {
	if (bus.initialized)
		return false;

	bus.addresses = malloc(sizeof(busAddr_t));
	if (bus.addresses == NULL)
		return false;
	bus.size = 1;

	bus.addresses->start = 0x0000;
	bus.addresses->stop = 0xFFFF;
	bus.addresses->component = NULL;

	bus.initialized = true;
	return true;
}

bool bus_destroy() {
	if (!bus.initialized)
		return false;

	free(bus.addresses);
	bus.addresses = NULL;
	bus.initialized = false;

	return true;
}

bool bus_add(const component_t* component, const uint16_t start, const uint16_t stop) {
	if (!bus.initialized)
		return false;

	if (start > stop)
		return bus_add(component, stop, start);

	// if start is 0x0000 and end is 0xFFFF
	// optimize by removing the entire list, and adding a new single entry
	if (start == 0x0000 && stop == 0xFFFF) {
		busAddr_t* newList = malloc(sizeof(busAddr_t));
		if (newList == NULL)
			return false;

		newList->start = 0x0000;
		newList->stop = 0xFFFF;
		newList->component = component;

		free(bus.addresses);
		bus.addresses = newList;
		bus.size = 1;
		return true;
	}

	busAddr_t* startAddr = NULL;
	busAddr_t* stopAddr = NULL;

	for (size_t i = 0; i < bus.size; i++) {
		const busAddr_t* current = bus.addresses + i;

		if (start >= current->start && start <= current->stop)
			startAddr = current;

		if (stop >= current->start && stop <= current->stop)
			stopAddr = current;
	}

	if (startAddr == NULL || stopAddr == NULL)	// shouldn't be possible, bus should always at least have a range of 0x0000-0xFFFF
		return false;							// keep this for a sanity check

	if (startAddr > stopAddr)	// if hit it would indicate that either stop is before end
		return false;			//   (which shouldn't be possible with earlier check)
								// or the bus is not in order
								//   (which shouldn't be possible either with proper use of this function)
								// keep this for a sanity check

	const bool addBefore = start != startAddr->start;
	const bool addAfter = stop != stopAddr->stop;
	const size_t startIndex = startAddr - bus.addresses;
	const size_t stopIndex = stopAddr - bus.addresses;
	const size_t distance = stopAddr - startAddr;

	if (distance == 0) {
		// early return for simple change
		if (!addBefore && !addAfter) {
			startAddr->component = component;

			return true;
		}

		// allocate new memory
		{
			const busAddr_t* newList = realloc(bus.addresses, sizeof(busAddr_t) * (bus.size + addBefore + addAfter));
			if (newList == NULL)
				return false;

			bus.addresses = newList;
		}

		// reset startAddr ptr because data might have moved
		startAddr = bus.addresses + startIndex + addBefore;
		memmove(startAddr + addAfter, startAddr - addBefore, sizeof(busAddr_t) * (bus.size - startIndex));

		// update altered bus ranges
		startAddr->start = start;
		startAddr->stop = stop;
		startAddr->component = component;
		if (addBefore)
			(startAddr - 1)->stop = start - 1;
		if (addAfter)
			(startAddr + 1)->start = stop + 1;

		bus.size += addBefore + addAfter;

		return true;
	}
	if (distance == 1) {
		if (addBefore ^ addAfter) {
			// if distance between start and end is 1 AND either one BUT NOT both need to be split
			// optimize by altering size of start and end
			// including changing the functions of the one which was aligned to an edge
			if (addBefore) {
				startAddr->stop = start - 1;

				stopAddr->start = start;
				startAddr->component = component;
			}
			if (addAfter) {
				stopAddr->start = stop + 1;

				startAddr->stop = stop;
				startAddr->component = component;
			}
			return true;
		}
	}
	if (distance == 2) {
		if (addBefore && addAfter) {
			// if distance between start and end is 2 AND they both need to be split
			// optimize by altering size of start, end and the one between
			// including changing the functions of the center one
			busAddr_t* center = startAddr + 1;

			startAddr->stop = start - 1;

			center->start = start;
			center->stop = stop;
			startAddr->component = component;

			stopAddr->start = stop + 1;

			return true;
		}
	}

	const size_t newSize = bus.size + (addBefore + addAfter) - (stopIndex - startIndex);
	const size_t newIndex = startIndex + addBefore;
	busAddr_t* newList = malloc(newSize * sizeof(busAddr_t));

	if (newList == NULL)
		return false;

	if (startIndex != 0 || addBefore)
		memmove(
			newList,
			bus.addresses,
			(startIndex + addBefore) * sizeof(busAddr_t)
		);
	if (stopIndex != bus.size - 1 || addAfter)
		memmove(
			newList + startIndex + 1 + addBefore,
			stopAddr + 1 - addAfter,
			(bus.size - stopIndex - 1 + addAfter) * sizeof(busAddr_t)
		);

	newList[newIndex].start = start;
	newList[newIndex].stop = stop;
	newList[newIndex].component = component;
	if (newIndex != 0)
		newList[newIndex - 1].stop = start - 1;
	if (newIndex != newSize - 1)
		newList[newIndex + 1].start = stop + 1;

	free(bus.addresses);
	bus.addresses = newList;
	bus.size = newSize;

	return true;
}

uint8_t bus_read(const uint16_t fullAddr) {
	for (size_t i = 0; i < bus.size; i++) {
		const busAddr_t* current = bus.addresses + i;

		if (fullAddr >= current->start && fullAddr <= current->stop) {
			if (current->component->readFunc)
				return current->component->readFunc(current->component, (addr_t) { fullAddr, fullAddr - current->start });

			return 0;
		}
	}

	return 0;
}

void bus_write(const uint16_t fullAddr, const uint8_t data) {
	for (size_t i = 0; i < bus.size; i++) {
		const busAddr_t* current = bus.addresses + i;

		if (fullAddr >= current->start && fullAddr <= current->stop) {
			if (current->component->writeFunc)
				current->component->writeFunc(current->component, (addr_t) { fullAddr, fullAddr - current->start }, data);
			return;
		}
	}
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

	printf("bus size: %zu", bus.size);
	for (size_t i = 0; i < bus.size; i++) {
		const busAddr_t* current = bus.addresses + i;

		const component_t component = current->component ? *current->component : (component_t) { 0 };

		printf("\n");

		printBin(current->start);
		printf("\t%04X", current->start);
		if (current->start != current->stop) {
			printf("\n........ ........\t....\t%s\tr%p w%p\n", component.name, component.readFunc, component.writeFunc);
			printBin(current->stop);
			printf("\t%04X\n", current->stop);
		} else
			printf("\t%s\tr%p w%p\n", component.name, component.writeFunc, component.writeFunc);
	}
}
