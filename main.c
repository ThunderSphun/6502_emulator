#include "bus.h"
#include "memory.h"
#include "cpu.h"
#include "clock.h"

#include <stdio.h>

int main() {
	bus_init();

	device_t ram = memory_init(0x10000, true);

	if (!bus_add(&ram, 0x0000, 0xFFFF)) {
		memory_destroy(ram);
		bus_destroy();
		return -1;
	}
	if (!memory_randomize(&ram)) {
		memory_destroy(ram);
		bus_destroy();
		return -1;
	}
	const char* binFile = "test_6502.bin";
	if (!memory_loadFile(&ram, binFile, 0x000a)) {
		memory_destroy(ram);
		bus_destroy();
		return -1;
	}

	clock_reset();
	clock_run(1000);

	memory_destroy(ram);
	bus_destroy();

	return 0;
}
