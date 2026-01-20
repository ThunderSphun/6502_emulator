#include "ram.h"

#include <stdio.h>

struct ram {
	uint8_t* data;
	size_t size;
};

inline struct ram* getRam(const component_t* const component) {
	return ((struct ram*) (component->component_data));
}

uint8_t ram_read(const component_t* const component, const addr_t addr) {
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

void ram_write(const component_t* const component, const addr_t addr, const uint8_t data) {
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

component_t ram_init(const size_t size) {
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

bool ram_randomize(const component_t* const component) {
	if (getRam(component) == NULL)
		return false;

	for (size_t i = 0; i < getRam(component)->size; i++)
		getRam(component)->data[i] = (uint8_t) ((rand() / (float) RAND_MAX) * 0xFF);

	return true;
}

bool ram_set(const component_t* const component, const uint16_t addr, const size_t size, const uint8_t* data) {
	if (getRam(component) == NULL)
		return false;

	if (addr > getRam(component)->size)
		return false;

	if (getRam(component)->size - addr < size)
		return false;

	memcpy(getRam(component)->data + addr, data, size);

	return true;
}

bool ram_loadFile(const component_t* const component, const char* fileName, const uint16_t addr) {
	if (getRam(component) == NULL)
		return false;

	if (addr > getRam(component)->size)
		return false;

	FILE* file = fopen(fileName, "rb");
	if (!file)
		return false;

	fseek(file, 0, SEEK_END);
	size_t fileSize = (size_t) ftell(file);
	rewind(file);

	if (getRam(component)->size - addr < fileSize) {
		fclose(file);
		return false;
	}

	fread(getRam(component)->data + addr, 1, fileSize, file);
	fclose(file);

	return true;
}
