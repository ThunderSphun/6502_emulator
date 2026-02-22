#pragma once

#include <stdint.h>

typedef struct device device_t;

typedef struct {
	const uint16_t full;
	const uint16_t relative;
} addr_t;

typedef uint8_t (*deviceRead)(const device_t* const device, const addr_t addr);
typedef void (*deviceWrite)(const device_t* const device, const addr_t addr, const uint8_t data);

struct device {
	const void* const device_data;
	const char* const name;
	const deviceRead readFunc;
	const deviceWrite writeFunc;
};
