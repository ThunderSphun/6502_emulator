#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef struct component component_t;

typedef struct addr {
	const uint16_t full;
	const uint16_t relative;
} addr_t;

typedef uint8_t (*componentRead)(component_t* component, addr_t addr);
typedef void (*componentWrite)(component_t* component, addr_t addr, uint8_t data);

struct component {
	const void* const component_data;
	const componentRead readFunc;
	const componentWrite writeFunc;
};
