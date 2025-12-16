#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef uint8_t (*busReadFunc)(uint16_t addr);
typedef void (*busWriteFunc)(uint16_t addr, uint8_t data);

bool bus_init();
bool bus_add(busReadFunc readFunc, busWriteFunc writeFunc, uint16_t begin, uint16_t end);
bool bus_destroy();

void bus_print();

uint8_t busRead(uint16_t addr);
void busWrite(uint16_t addr, uint8_t data);
