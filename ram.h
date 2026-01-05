#pragma once

#include "component.h"

component_t ram_init(const uint16_t size);
bool ram_destroy(component_t component);

bool ram_randomize(const component_t* const component);
