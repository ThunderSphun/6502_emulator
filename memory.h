#pragma once

#include "device.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

device_t memory_init(const size_t size, bool canWrite);
bool memory_destroy(device_t device);

bool memory_randomize(const device_t* const device);
bool memory_set(const device_t* const device, const uint16_t addr, const size_t size, const uint8_t* data);
bool memory_loadFile(const device_t* const device, const char* fileName, const uint16_t addr);