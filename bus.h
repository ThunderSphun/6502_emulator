#pragma once

#include "component.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

bool bus_init();
bool bus_destroy();

bool bus_add(const component_t* const component, const uint16_t start, const uint16_t stop);

uint8_t bus_read(const uint16_t fullAddr);
void bus_write(const uint16_t fullAddr, const uint8_t data);

void bus_print();
