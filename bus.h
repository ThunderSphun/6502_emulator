#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct bus bus_t;
typedef struct busAddr busAddr_t;

typedef uint8_t (*busReadFunc)(uint16_t addr);
typedef void (*busWriteFunc)(uint16_t addr, uint8_t data);

struct bus {
	busAddr_t* addresses;
	size_t size;
};

struct busAddr {
	uint16_t begin;
	uint16_t end;
	busReadFunc read;
	busWriteFunc write;
};

bool bus_init(bus_t* bus);
bool bus_destroy(bus_t* bus);
