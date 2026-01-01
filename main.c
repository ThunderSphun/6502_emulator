#include "bus.h"
#include "rom.h"
#include "ram.h"

int main() {
	bus_init();
	component_t ram = ram_init(0x8000);
	component_t rom = rom_init(0x8000);

	bus_add(&ram, 0x0000, 0x7FFF);
	bus_add(&rom, 0x8000, 0xFFFF);
	bus_print();

	rom_destroy(rom);
	ram_destroy(ram);
	bus_destroy();

	return 0;
}
