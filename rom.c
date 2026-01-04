#include "rom.h"

#include <stdio.h>

struct rom {
	uint8_t* data;
	uint16_t size;
};

struct rom* getRom(component_t* component) {
	return ((struct rom*) (component->component_data));
}

uint8_t rom_read(component_t* component, addr_t addr) {
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

component_t rom_init(uint16_t size) {
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
