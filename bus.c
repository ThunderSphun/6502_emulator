#include "bus.h"

#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define VERBOSE

typedef struct {
	uint16_t begin;
	uint16_t end;
	uint16_t base;
	const device_t* device;
} region_t;

static struct {
	region_t* regions;
	size_t size;
} bus = { 0 };

#ifdef VERBOSE
static uint8_t read(const device_t* device, addr_t address) {
	(void) device; (void) address;
	printf("READ:\ndevice: %p\naddress: {%04X, %04X}\nEND READ\n", device, address.full, address.relative);
	return 0x55;
}

static void write(const device_t* device, addr_t address, uint8_t data) {
	(void) device; (void) address; (void) data;
	printf("WRITE:\ndevice: %p\naddress: {%04X, %04X}\nvalue: %02X\nEND WRITE\n", device, address.full, address.relative, data);
}

static device_t nullDevice = (device_t) { .name = "null", .readFunc = read, .writeFunc = write };
#else
static device_t nullDevice = (device_t) {.name = "null" };
#endif

#define SEARCH(addr) bsearch(&addr, bus.regions, bus.size, sizeof(region_t), region_search)
static int region_search(const void* addr, const void* region) {
	if (*(uint16_t*) addr < ((region_t*) region)->begin)
		return -1;
	if (*(uint16_t*) addr > ((region_t*) region)->end)
		return 1;

	return 0;
}

bool bus_init() {
	if (bus.regions)
		return false;

	bus.regions = malloc(sizeof(region_t));
	if (bus.regions == NULL)
		return false;
	bus.size = 1;

	bus.regions[0] = (region_t) { .begin = 0x0000, .end = 0xFFFF, .base = 0x0000, .device = &nullDevice };

	return true;
}

bool bus_destroy() {
	if (!bus.regions)
		return false;

	free(bus.regions);
	bus.regions = NULL;

	return true;
}

bool bus_add(const device_t* const device, const uint16_t begin, const uint16_t end) {
	if (!bus.regions)
		return false;

	if (begin > end)
		return bus_add(device, end, begin);

	if (device == NULL)
		return bus_add(&nullDevice, begin, end);

	region_t* newRegions = malloc(sizeof(region_t) * (bus.size + 2));
	if (newRegions == NULL) {
#ifdef VERBOSE
		printf("malloc for bus_add failed\n");
#endif
		return false;
	}

#ifdef VERBOSE
	printf("adding a new device at range [%04X, %04X]\n", begin, end);
#endif

	size_t newSize = 0;
	for (size_t i = 0; i < bus.size; i++) {
		region_t* region = bus.regions + i;

#ifdef VERBOSE
#define PRINT_REGION() \
		printf("region %li/%li {begin: %04X, end: %04X, base: %04X} ", \
			i + 1, bus.size, region->begin, region->end, region->base);
#endif

if (end < region->begin || begin > region->end) {
#ifdef VERBOSE
			PRINT_REGION();
			printf("can be kept the same\n");
#endif
			newRegions[newSize++] = *region;
			continue;
		}

		if (begin > region->begin) {
#ifdef VERBOSE
			PRINT_REGION();
			printf("needs a new region before\n");
#endif
			newRegions[newSize] = *region;
			newRegions[newSize].end = begin - 1;
			newSize++;
		}

#ifdef VERBOSE
			PRINT_REGION();
			printf("adds a new region\n");
#endif

		newRegions[newSize++] = (region_t) {
			.begin = begin,
			.base = 0,
			.end = end,
			.device = device,
		};

		if (end < region->end) {
#ifdef VERBOSE
			PRINT_REGION();
			printf("needs a new region after\n");
#endif
			newRegions[newSize] = *region;
			newRegions[newSize].begin = end + 1;
			newRegions[newSize].base += newRegions[newSize].begin - region->begin;
			newSize++;
		}

#ifdef VERBOSE
#undef PRINT_REGION
#endif
	}

	size_t write = 0;
	for (size_t read = 1; read < newSize; read++) {
		region_t* prev = newRegions + write;
		region_t* curr = newRegions + read;

		if (prev->device == curr->device)
			prev->end = curr->end;
		else
			newRegions[++write] = *curr;
	}

	region_t* shrunkRegions = realloc(newRegions, sizeof(region_t) * (write + 1));

	free(bus.regions);
	bus.regions = shrunkRegions ? shrunkRegions : newRegions;
	bus.size = write + 1;

	return true;
}

uint8_t bus_read(const uint16_t fullAddr) {
#ifdef VERBOSE
	printf("searching for region to read at %04X\n", fullAddr);
#endif

	region_t* result = SEARCH(fullAddr);
	if (result) {
#ifdef VERBOSE
		printf("found region %p\n", result);
#endif
		if (result->device->readFunc) {
			uint8_t readVal = result->device->readFunc(result->device, (addr_t) {fullAddr, result->base + (fullAddr - result->begin)});

#ifdef VERBOSE
			printf("read value %02X\n", readVal);
#endif
			return readVal;
		}
#ifdef VERBOSE
		printf("region cannot be read\n");
#endif
		return 0;
	}

#ifdef VERBOSE
	printf("couldn't find region\n");
#endif
	return 0;
}

void bus_write(const uint16_t fullAddr, const uint8_t data) {
#ifdef VERBOSE
	printf("searching for region to write at %04X\n", fullAddr);
#endif

	region_t* result = SEARCH(fullAddr);
	if (result) {
#ifdef VERBOSE
		printf("found region %p\n", result);
#endif
		if (result->device->writeFunc) {
			result->device->writeFunc(result->device, (addr_t) {fullAddr, result->base + (fullAddr - result->begin)}, data);
#ifdef VERBOSE
			printf("written value\n");
#endif
			return;
		}

#ifdef VERBOSE
		printf("region cannot be written to\n");
#endif
		return;
	}

#ifdef VERBOSE
		printf("couldn't find region\n");
#endif
}

void bus_print() {
	if (!bus.regions) {
		printf("bus not initialized");
		return;
	}

	printf("bus size: %zu", bus.size);
	for (size_t i = 0; i < bus.size; i++) {
		const region_t* current = bus.regions + i;

		const device_t device = *current->device;

		printf("\n");

		printf("%8s ", byteToBinStr(current->begin >> 8));
		printf("%8s", byteToBinStr(current->begin & 0xFF));
		printf("\t%04X", current->begin);
		if (current->begin != current->end) {
			printf("\n........ ........\t....\t%s\tr%p w%p\n", device.name, device.readFunc, device.writeFunc);
			printf("%8s ", byteToBinStr(current->end >> 8));
			printf("%8s", byteToBinStr(current->end & 0xFF));
			printf("\t%04X\n", current->end);
		} else
			printf("\t%s\tr%p w%p\n", device.name, device.readFunc, device.writeFunc);
	}
}
