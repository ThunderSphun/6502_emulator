#pragma once

#include "component.h"

component_t ram_init(uint16_t size);
bool ram_destroy(component_t component);

bool ram_set(component_t* component, uint16_t addr, uint16_t size, uint8_t* data);
