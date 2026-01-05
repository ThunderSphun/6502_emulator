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

#pragma region addressingModes
// the following functions all implement logic for the addressing mode
// if they return true an additional clockcycle should be taken


// implied addressing mode
// the operand is implied by the instruction
bool am_imp() {
	return 0;
}

// immediate addressing mode
// the operand is provided directly after the instruction
bool am_imm() {
	return 0;
}

// relative addressing mode
// the operand provided is a signed byte
// this byte is added to the program counter to get the address to branch to
// this mode is only allowed for the branch instructions
bool am_rel() {
	return 0;
}

// zero-page addressing mode
// a byte is provided directly after the instruction which contains an offset in the zero page
// the absolute address would be $00XX
// this makes the operation faster, and shorter
// the instruction takes only 2 bytes, instead of 3 for a full address
bool am_zp0() {
	return 0;
}

// zero-page addressing mode offset by X
// a byte is provided directly after the instruction which contains an offset in the zero page
// this memory location is incremented by X, and the value at that memory location is used as the operand
// this can be used to loop through a set of data, aka an array
// this makes the operation faster, and shorter
// the instruction takes only 2 bytes, instead of 3 for a full address
// the result of the addition wraps around, so the byte read will always be inside the zero page.
bool am_zpx() {
	return 0;
}

// zero-page addressing mode offset by Y
// a byte is provided directly after the instruction which contains an offset in the zero page
// this memory location is incremented by Y, and the value at that memory location is used as the operand
// this can be used to loop through a set of data, aka an array
// this makes the operation faster, and shorter
// the instruction takes only 2 bytes, instead of 3 for a full address
// the result of the addition wraps around, so the byte read will always be inside the zero page.
// this addressing mode is only used if the register used is X (LDX, STX), so X cannot be used
bool am_zpy() {
	return 0;
}

#ifdef WDC
// zero-page indirect addressing mode
// a byte is provided directly after the instruction which contains an offset in the zero page
// this memory location provides 2 byte to actually use, in the format $LLHH
bool am_zpi() {
	return 0;
}
#endif

// absolute addressing mode
// an absolute memory location is provided
// the value at this memory location is used as the operand
bool am_abs() {
	return 0;
}

#ifdef WDC
// absolute indexed indirect addressing mode
// an absolute memory location is provided
// this memory location is incremented by X, this memory location provides 2 bytes to actually use, in the format $LLHH
// this mode is generally only used for JMP
bool am_abi() {
	return 0;
}
#endif

// absolute addressing mode offset by X
// an absolute memory location is provided
// this memory location is incremented by X, and the value at that memory location is used as the operand
// this can be used to loop through a set of data, aka an array
bool am_abx() {
	return 0;
}

// absolute addressing mode offset by Y
// an absolute memory location is provided
// this memory location is incremented by Y, and the value at that memory location is used as the operand
// this can be used to loop through a set of data, aka an array
bool am_aby() {
	return 0;
}

// indirect addressing mode
// an absolute memory location is provided
// this memory location provides 2 byte to actually use, in the format $LLHH
// this mode is generally only used for JMP
bool am_ind() {
	return 0;
}

// pre-indexed indirect addressing mode
// a byte is provided, which describes the offset in the zero page
// from this zero page address two bytes are read ($LLHH), which gets incremented by X
// this increased memory address points to the memory address ($LLHH) where the actual data is stored
bool am_inx() {
	return 0;
}

// post-indexed indirect addressing mode
// a byte is provided, which describes the offset in the zero page
// from this zero page address two bytes are read ($LLHH)
// this memory address points to the memory address ($LLHH), which gets incremented with Y
// this address is where the actual data is stored
bool am_iny() {
	return 0;
}
#pragma endregion addressingModes

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

	const uint8_t instruction = bus_read(registers.PC++);
	// use instruction

	// set cycles from instruction
	cycles = 1;
}

// runs instruction and all clockcycles required
void cpu_runInstruction() {
	// make sure previous instruction is 'finished'
	while (cycles > 0)
		cpu_clock();

	// execute instruction
	cpu_clock();

	// make sure current instruction is 'finished'
	while (cycles > 0)
		cpu_clock();
}
