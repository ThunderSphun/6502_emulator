#pragma once

#include "component.h"

component_t ram_init(const size_t size);
bool ram_destroy(component_t component);

bool ram_randomize(const component_t* const component);
bool ram_set(const component_t* const component, const uint16_t addr, const size_t size, const uint8_t* data);
