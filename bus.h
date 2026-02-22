#pragma once

#include "device.h"

#include <stdbool.h>

bool bus_init();
bool bus_destroy();

/// attaches a device to the bus, spanning range [begin - end]
/// if a device already exists at that place, it will be hidden for the overlapping address range
/// if a device is fully overwritten the bus has forgotten about this device.
/// IT IS NOT DELETED BY THE BUS, THE BUS ONLY KEEPS A POINTER TO THE DEVICE
bool bus_add(const device_t* const device, const uint16_t begin, const uint16_t end);

/// functions to interact with the bus
/// read and get are used to get data from a device at that address
/// write and place stores data in a device at that address
/// read and write place the data actually in the device, and make the device aware of it
/// get and place act as a silent counterpart.
/// these do modify the data of the connected device, but shouldn't notify the device of this change
/// these could be used to observe the data on the bus, or modifying data which otherwise would make the device response.
/// get and place fall back to read and write if the function pointers in device are NULL
/// read and get return 0 if no device was found, or the device could not be read
uint8_t bus_read(const uint16_t fullAddr);
uint8_t bus_get(const uint16_t fullAddr);
void bus_write(const uint16_t fullAddr, const uint8_t data);
void bus_place(const uint16_t fullAddr, const uint8_t data);

void bus_print();
