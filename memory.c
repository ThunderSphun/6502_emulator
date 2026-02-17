#include "memory.h"

#include <stdio.h>

struct memory {
	uint8_t* data;
	size_t size;
};

#define GET_DATA(component) ((struct memory*) (component->component_data))

uint8_t memory_read(const component_t* const component, const addr_t addr) {
	if (GET_DATA(component) == NULL) {
		printf("ram is not initialized\n");
		return 0;
	}

	if (GET_DATA(component)->size < addr.full) {
		printf("outside of ram range\n");
		return 0;
	}

	return GET_DATA(component)->data[addr.full];
}

void memory_write(const component_t* const component, const addr_t addr, const uint8_t data) {
	if (GET_DATA(component) == NULL) {
		printf("ram is not initialized\n");
		return;
	}

	if (GET_DATA(component)->size < addr.full) {
		printf("outside of ram range\n");
		return;
	}

	GET_DATA(component)->data[addr.full] = data;
}

component_t memory_init(const size_t size, bool canWrite) {
	struct memory* memory = malloc(sizeof(struct memory));
	if (memory == NULL)
		return (component_t) { 0 };
	memory->data = malloc(size * sizeof(uint8_t));
	if (memory->data == NULL) {
		free(memory);
		return (component_t) { 0 };
	}
	memory->size = size;

	return (component_t) { memory, "memory", memory_read, canWrite ? memory_write : NULL };
}

bool memory_destroy(component_t component) {
	if (GET_DATA((&component)) == NULL)
		return false;

	free(GET_DATA((&component))->data);
	free(GET_DATA((&component)));

	return true;
}

bool memory_randomize(const component_t* const component) {
	if (GET_DATA(component) == NULL)
		return false;

	for (size_t i = 0; i < GET_DATA(component)->size; i++)
		GET_DATA(component)->data[i] = (uint8_t) ((rand() / (float) RAND_MAX) * 0xFF);

	return true;
}

bool memory_set(const component_t* const component, const uint16_t addr, const size_t size, const uint8_t* data) {
	if (GET_DATA(component) == NULL)
		return false;

	if (addr > GET_DATA(component)->size)
		return false;

	if (GET_DATA(component)->size - addr < size)
		return false;

	memcpy(GET_DATA(component)->data + addr, data, size);

	return true;
}

bool memory_loadFile(const component_t* const component, const char* fileName, const uint16_t addr) {
	if (GET_DATA(component) == NULL)
		return false;

	if (addr > GET_DATA(component)->size)
		return false;

	FILE* file = fopen(fileName, "rb");
	if (!file) {
		printf("could not open file %s\n", fileName);
		return false;
	}

	fseek(file, 0, SEEK_END);
	size_t fileSize = (size_t) ftell(file);
	rewind(file);

	if (GET_DATA(component)->size - addr < fileSize) {
		fclose(file);
		return false;
	}

	fread(GET_DATA(component)->data + addr, 1, fileSize, file);
	fclose(file);

	return true;
}
