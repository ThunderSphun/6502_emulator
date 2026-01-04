#pragma once

#include "component.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

bool bus_init();
bool bus_destroy();

bool bus_add(component_t* component, uint16_t start, uint16_t stop);

uint8_t bus_read(uint16_t fullAddr);
void bus_write(uint16_t fullAddr, uint8_t data);

void bus_print();
