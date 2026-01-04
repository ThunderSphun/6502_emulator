#include "cpu.h"

#include "bus.h"

struct registers {
	union {
		struct {
			uint8_t PC_LO;
			uint8_t PC_HI;
		};
		uint16_t PC;
	};
	uint8_t A;
	uint8_t X;
	uint8_t Y;
	union {
		struct {
			bool C : 1;
			bool Z : 1;
			bool I : 1;
			bool D : 1;
			bool B : 1;
			bool _ : 1;
			bool V : 1;
			bool N : 1;
		};
		uint8_t P;
	};
	uint8_t SP;
} registers;

size_t cycles = 0;

void push(uint8_t data) {
	bus_write(0x0100 | registers.SP--, data);
}

void cpu_irq() {
	if (!registers.I)
		return;

	push(registers.PC >> 8);
	push(registers.PC & 0xFF);
	push(registers.P);

	registers.PC_LO = bus_read(0xFFFE);
	registers.PC_HI = bus_read(0xFFFF);

	registers.I = true;

	cycles = 7;
}

void cpu_reset() {
	registers.PC_LO = bus_read(0xFFFC);
	registers.PC_HI = bus_read(0xFFFD);

	registers.SP = (uint8_t) ((rand() / (float) RAND_MAX) * 0xFF);

	registers._ = 1;
	registers.B = 1;
	registers.D = 0;
	registers.I = 1;

	cycles = 7;
}

void cpu_nmi() {
	push(registers.PC >> 8);
	push(registers.PC & 0xFF);
	push(registers.P);

	registers.PC_LO = bus_read(0xFFFA);
	registers.PC_HI = bus_read(0xFFFB);

	registers.I = true;

	cycles = 7;
}

// run single instruction
// waits to run the instruction until no more cycles need to be consumed
// afterwards runs entire instruction in a single clockcycle
// if a cycle needs to be consumed, this function returns early and consumes a single cycle
void cpu_clock() {
	if (cycles-- != 0)
		return;

	uint8_t instruction = bus_read(registers.PC++);
	// use instruction

	// set cycles from instruction
	cycles = 1;
}

// runs instruction and all clockcycles required
void cpu_runInstruction() {
	// make sure last instruction is 'finished'
	while (cycles > 0)
		cpu_clock();

	// execute instruction
	cpu_clock();

	// make sure current instruction is 'finished'
	while (cycles > 0)
		cpu_clock();
}
