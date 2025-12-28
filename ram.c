#include "ram.h"

#include <stdio.h>

static struct ram {
	uint8_t* data;
	uint16_t size;
	bool initialized;
} ram = { 0 };

bool ram_init(uint16_t size) {
	if (ram.initialized)
		return false;

	ram.data = malloc(size * sizeof(uint8_t));
	if (ram.data == NULL)
		return false;
	ram.size = size;
	ram.initialized = true;

	return true;
}

bool ram_destroy() {
	if (!ram.initialized)
		return false;

	free(ram.data);
	ram.initialized = false;
	return true;
}

uint8_t ram_read(addr_t addr) {
	if (!ram.initialized) {
		printf("ram is not initialized");
		return 0;
	}

	if (addr.full >= 0x8000) {
		printf("ram is disabled");
		return 0;
	}
	return ram.data[addr.relative];
}

void ram_write(addr_t addr, uint8_t data) {
	if (!ram.initialized) {
		printf("ram is not initialized");
		return;
	}

	if (addr.full >= 0x8000)
		printf("ram is disabled");

	ram.data[addr.relative] = data;
}
