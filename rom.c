#include "rom.h"

#include <stdio.h>

static struct rom {
	uint8_t* data;
	uint16_t size;
	bool initialized;
} rom = { 0 };

bool rom_init(uint16_t size) {
	if (rom.initialized)
		return false;

	rom.data = malloc(size * sizeof(uint8_t));
	if (rom.data == NULL)
		return false;
	rom.size = size;
	rom.initialized = true;

	return true;
}

bool rom_destroy() {
	if (!rom.initialized)
		return false;

	free(rom.data);
	rom.initialized = false;
	return true;
}

uint8_t rom_read(addr_t addr) {
	if (!rom.initialized) {
		printf("rom is not initialized");
		return 0;
	}

	if (addr.full >= 0x8000) {
		printf("rom is disabled");
		return 0;
	}
	return rom.data[addr.relative];
}
