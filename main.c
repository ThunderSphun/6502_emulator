#include "bus.h"
#include "rom.h"
#include "ram.h"

int main() {
	bus_init();
	ram_init(0x8000);
	rom_init(0x8000);

	bus_add(ram_read, ram_write, 0x0000, 0x7FFF);
	bus_add(rom_read, NULL, 0x8000, 0xFFFF);
	bus_print();

	rom_destroy();
	ram_destroy();
	bus_destroy();

	return 0;
}
