#pragma once

#include "device.h"

#include <stdbool.h>

bool bus_init();
bool bus_destroy();

bool bus_add(const device_t* const device, const uint16_t begin, const uint16_t end);

uint8_t bus_read(const uint16_t fullAddr);
uint8_t bus_get(const uint16_t fullAddr);
void bus_place(const uint16_t fullAddr, const uint8_t data);
void bus_write(const uint16_t fullAddr, const uint8_t data);

void bus_print();
