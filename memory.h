#pragma once

#include "device.h"

#include <stdbool.h>
#include <stddef.h>

device_t memory_init(const size_t size, const bool canWrite);
bool memory_destroy(device_t device);

bool memory_randomize(deviceRef_t device);
bool memory_set(deviceRef_t device, const uint16_t addr, const size_t size, const uint8_t* data);
bool memory_loadFile(deviceRef_t device, const char* fileName, const uint16_t addr);