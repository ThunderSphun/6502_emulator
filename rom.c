#include "rom.h"

#include <stdio.h>

struct rom {
	uint8_t* data;
	size_t size;
};

struct rom* getRom(const component_t* const component) {
	return ((struct rom*) (component->component_data));
}

uint8_t rom_read(const component_t* const component, const addr_t addr) {
	if (getRom(component) == NULL) {
		printf("rom is not initialized\n");
		return 0;
	}

	if (getRom(component)->size < addr.relative) {
		printf("outside of rom range\n");
		return 0;
	}

	return getRom(component)->data[addr.relative];
}

component_t rom_init(const size_t size) {
	struct rom* rom = malloc(sizeof(struct rom));
	if (rom == NULL)
		return (component_t) { 0 };
	rom->data = malloc(size * sizeof(uint8_t));
	if (rom->data == NULL) {
		free(rom);
		return (component_t) { 0 };
	}
	rom->size = size;

	return (component_t) { rom, "rom", rom_read, NULL };
}

bool rom_destroy(component_t component) {
	if (getRom(&component) == NULL)
		return false;

	free(getRom(&component)->data);
	free(getRom(&component));

	return true;
}

bool rom_set(const component_t* const component, const uint16_t addr, const size_t size, const uint8_t* data) {
	if (getRom(component) == NULL)
		return false;

	if (addr > getRom(component)->size)
		return false;

	if (getRom(component)->size - addr < size)
		return false;

	memcpy(getRom(component)->data + addr, data, size);

	return true;
}