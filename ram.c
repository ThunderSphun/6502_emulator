#include "ram.h"

#include <stdio.h>

struct ram {
	uint8_t* data;
	uint16_t size;
};

struct ram* getRam(component_t* component) {
	return ((struct ram*) (component->component_data));
}

uint8_t ram_read(component_t* component, addr_t addr) {
	if (getRam(component) == NULL) {
		printf("ram is not initialized\n");
		return 0;
	}

	if (getRam(component)->size < addr.relative) {
		printf("outside of ram range\n");
		return 0;
	}

	return getRam(component)->data[addr.relative];
}

void ram_write(component_t* component, addr_t addr, uint8_t data) {
	if (getRam(component) == NULL) {
		printf("ram is not initialized\n");
		return;
	}

	if (getRam(component)->size < addr.relative) {
		printf("outside of ram range\n");
		return;
	}

	getRam(component)->data[addr.relative] = data;
}

component_t ram_init(uint16_t size) {
	struct ram* ram = malloc(sizeof(struct ram));
	if (ram == NULL)
		return (component_t) { 0 };
	ram->data = malloc(size * sizeof(uint8_t));
	if (ram->data == NULL) {
		free(ram);
		return (component_t) { 0 };
	}
	ram->size = size;

	return (component_t) { ram, "ram", ram_read, ram_write };
}

bool ram_destroy(component_t component) {
	if (getRam(&component) == NULL)
		return false;

	free(getRam(&component)->data);
	free(getRam(&component));

	return true;
}
