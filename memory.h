#pragma once

#include "component.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

component_t memory_init(const size_t size, bool canWrite);
bool memory_destroy(component_t component);

bool memory_randomize(const component_t* const component);
bool memory_set(const component_t* const component, const uint16_t addr, const size_t size, const uint8_t* data);
bool memory_loadFile(const component_t* const component, const char* fileName, const uint16_t addr);