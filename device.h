#pragma once

#include <stdint.h>

typedef struct device device_t;

typedef struct {
	const uint16_t full;
	const uint16_t relative;
} addr_t;

typedef uint8_t (*deviceRead)(const device_t* const device, const addr_t addr);
typedef void (*deviceWrite)(const device_t* const device, const addr_t addr, const uint8_t data);

/// a device to be placed on the bus
/// a device can be anything connected to the bus, and provides a flexible interface
/// a device can have private info, stored in device_data, which can be modified/interacted with on a read/write
/// when the bus reads from this component, it should call readFunc and pass this device into the call
/// when the bus writes to this component, it should call writeFunc and pass this device into the call
/// if the bus wishes to silently read/write, it should use getFunc/placeFunc
/// readFunc should be NULL if it shouldn't have read capability
/// getFunc should be NULL if it shouldn't have get functionality OR it is the same as readFunc
/// writeFunc should be NULL if it shouldn't have write capability
/// placeFunc should be NULL if it shouldn't have place functionality OR it is the same as writeFunc
struct device {
	const void* const device_data;
	const char* const name;
	const deviceRead readFunc; // read with notify
	const deviceRead getFunc; // read without notify
	const deviceWrite writeFunc; // write with notify
	const deviceWrite placeFunc; // write without notify
};
