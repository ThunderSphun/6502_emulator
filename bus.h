#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef struct addr {
	const uint16_t full;
	const uint16_t relative;
} addr_t;

typedef uint8_t (*busReadFunc)(addr_t addr);
typedef void (*busWriteFunc)(addr_t addr, uint8_t data);

bool bus_init();
bool bus_destroy();

bool bus_add(busReadFunc readFunc, busWriteFunc writeFunc, uint16_t start, uint16_t stop);

uint8_t busRead(uint16_t fullAddr);
void busWrite(uint16_t fullAddr, uint8_t data);

void bus_print();
