#pragma once

#include <stdint.h>

typedef struct device device_t;
typedef const device_t* deviceRef_t;

typedef struct {
	uint16_t full;
	uint16_t relative;
} addr_t;

typedef uint8_t (*deviceRead)(deviceRef_t device, const addr_t addr);
typedef void (*deviceWrite)(deviceRef_t device, const addr_t addr, const uint8_t data);

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
	void* const device_data;
	const char* const name;
	const deviceRead readFunc;
	const deviceRead getFunc;
	const deviceWrite writeFunc;
	const deviceWrite placeFunc;
};
