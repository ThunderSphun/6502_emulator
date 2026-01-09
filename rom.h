#pragma once

#include "component.h"

component_t rom_init(const size_t size);
bool rom_destroy(component_t component);

bool rom_set(const component_t* const component, const uint16_t addr, const size_t size, const uint8_t* data);
