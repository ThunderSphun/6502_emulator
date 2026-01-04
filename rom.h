#pragma once

#include "component.h"

component_t rom_init(uint16_t size);
bool rom_destroy(component_t component);

bool rom_set(component_t* component, uint16_t addr, uint16_t size, uint8_t* data);
