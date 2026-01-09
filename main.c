#include "bus.h"
#include "rom.h"
#include "ram.h"
#include "cpu.h"

#include <stdio.h>

extern struct registers {
	uint16_t PC;
	uint8_t A;
	uint8_t X;
	uint8_t Y;
	uint8_t P;
	uint8_t SP;
} registers;

extern size_t cycles;

void printBusRange(const uint16_t start, const uint16_t stop) {
	if (start > stop) {
		printBusRange(stop, start);
		return;
	}
	for (size_t i = start & 0xFFF0; i <= stop; i += 0x0010) {
		if (i % 0x0100 == 0)
			printf("\n");

		printf("$%04X: ", (uint16_t) i);
		for (uint8_t j = 0; j < 0x10; j++) {
			if ((i + j) < start || (i + j) > stop)
				printf("__ ");
			else
				printf("%02X ", bus_read((uint16_t) i + j));
			if (j == 7)
				printf("\t");
		}
		printf("\n");
	}
	printf("\n");
}

void printStackPage() {
	printf("%02X", registers.SP);
	printBusRange(0x0100, 0x01FF);
}

int main() {
	bus_init();

	component_t ram = ram_init(0x10000);
	bus_add(&ram, 0x0000, 0xFFFF);

	uint8_t buffer[0x10000] = { 0 };
	{
		FILE* file = fopen("test.bin", "rb");
		if (file)
			fread(buffer, sizeof(buffer), 1, file);
		else {
			printf("couldnt open file\n");
			fclose(file);
			return -1;
		}
		fclose(file);
	}
	ram_set(&ram, 0x000a, 0x10000 - 0x000a, buffer);

	cpu_reset();
	registers.PC = 0x0400;
	while(registers.PC < 0x0410)
		cpu_runInstruction();

	ram_destroy(ram);
	bus_destroy();

	return 0;
}
