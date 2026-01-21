#include "cpu.h"

#include "bus.h"

#include <stdio.h>

// #define VERBOSE

#ifdef WDC
// Western Design Center included the bit branch/bit set instructions from the rockwel chips
#define ROCKWEL
#endif

#ifdef VERBOSE
#ifdef ROCKWEL
// account for bit number in some newer instructions
#define INSTRUCTION_NAME_LENGTH 4
#else
#define INSTRUCTION_NAME_LENGTH 3
#endif // ROCKWEL
#endif // VERBOSE

#ifdef VERBOSE
#define NO_IMPL() printf("instruction %-*s is not implemented\n", INSTRUCTION_NAME_LENGTH, instructions[opcodes[currentOpcode].instruction].name)
#else
#define NO_IMPL() printf("instruction %s is not implemented\n", __func__)
#endif

// addressing modes
enum {
#ifndef WDC
	// addressing mode for illegal opcode
	//   the WDC version of the 6502 has all illegal opcodes as implemented as nops
	AM_XXX,
#endif

#ifdef WDC
	AM_ABS, AM_ABSI, AM_ABSX, AM_ABSY,
#else
	AM_ABS, AM_ABSX, AM_ABSY,
#endif
	// accumulator addressing mode, same behaviour as implied, making the accumulator as the operator
	// this mode is however officially documented as a seperate addressing mode
	// the actual logic in this emulator will be done with implied
	AM_ACC,
	AM_IMM,
	AM_IMP,
	AM_IND, AM_INDX, AM_INDY,
	AM_REL,
#ifdef WDC
	// stack addressing mode, same behaviour as implied, making the stack pointer as the operator
	// this mode is however officially documented as a seperate addressing mode
	// the actual logic in this emulator will be done with implied
	AM_STK,
#endif
#ifdef WDC
	AM_ZPG, AM_ZPGI, AM_ZPGX, AM_ZPGY,
#else
	AM_ZPG, AM_ZPGX, AM_ZPGY,
#endif

	ADDRESS_MODE_COUNT
};

// instructions
enum {
#ifndef WDC
	// instruction for illegal opcode
	//   the WDC version of the 6502 has all illegal opcodes as implemented as nops
	IN_XXX,
#endif

	IN_NOP,
	// alu
	IN_ADC, IN_SBC,
	IN_INC, IN_INX, IN_INY,
	IN_DEC, IN_DEX, IN_DEY,
	IN_AND, IN_EOR, IN_ORA,
	IN_ASL, IN_LSR, IN_ROL, IN_ROR,
	// PC altering
#ifdef ROCKWEL
	IN_BBR0, IN_BBR1, IN_BBR2, IN_BBR3, IN_BBR4, IN_BBR5, IN_BBR6, IN_BBR7,
	IN_BBS0, IN_BBS1, IN_BBS2, IN_BBS3, IN_BBS4, IN_BBS5, IN_BBS6, IN_BBS7,
#endif
	IN_BCC, IN_BNE, IN_BMI, IN_BVC,
	IN_BCS, IN_BEQ, IN_BPL, IN_BVS,
#ifdef WDC
	IN_BRA,
#endif
	IN_BRK, IN_JSR, IN_JMP,
	IN_RTI, IN_RTS,
	// flags altering
	IN_BIT, IN_CMP, IN_CPX, IN_CPY,
	IN_CLC, IN_CLD, IN_CLI, IN_CLV,
	IN_SEC, IN_SED, IN_SEI,
	// load
	IN_LDA, IN_LDX, IN_LDY,
	IN_TAX, IN_TAY, IN_TSX,
	IN_TXA, IN_TYA, IN_TXS,
#ifdef WDC
	IN_PLA, IN_PLX, IN_PLY, IN_PLP,
	IN_TRB, IN_TSB,
#else
	IN_PLA, IN_PLP,
#endif
	// store
	IN_STA, IN_STX, IN_STY,
#ifdef ROCKWEL
	IN_RMB0, IN_RMB1, IN_RMB2, IN_RMB3, IN_RMB4, IN_RMB5, IN_RMB6, IN_RMB7,
	IN_SMB0, IN_SMB1, IN_SMB2, IN_SMB3, IN_SMB4, IN_SMB5, IN_SMB6, IN_SMB7,
#endif
#ifdef WDC
	IN_PHA, IN_PHX, IN_PHY, IN_PHP,
	IN_STZ,
#else
	IN_PHA, IN_PHP,
#endif
	// processor halting
#ifdef WDC
	IN_STP, IN_WAI,
#endif

	INSTRUCTION_COUNT
};

struct registers {
	union {
		struct {
			uint8_t PC_LO;
			uint8_t PC_HI;
		};
		uint16_t PC; // program counter
	};
	uint8_t A; // accumulator
	uint8_t X;
	uint8_t Y;
	union {
		struct {
			bool C : 1; // carry
			bool Z : 1; // zero
			bool I : 1; // interupt
			bool D : 1; // binary coded decimal (BDC)
			bool B : 1; // break
			bool _ : 1; // unused
			bool V : 1; // overflow
			bool N : 1; // negative
		};
		uint8_t flags;
	};
	uint8_t SP; // stack pointer
} registers;

struct opcode {
	const uint8_t instruction;
	const uint8_t addressMode;
	const uint8_t cycleCount;
} opcodes[256];

struct addressMode {
#ifdef VERBOSE
	const char name[5];
#endif
	void (*const func)();
} addressModes[ADDRESS_MODE_COUNT];

struct instruction {
#ifdef VERBOSE
	const char name[INSTRUCTION_NAME_LENGTH + 1];
#endif
	void (*const func)();
} instructions[INSTRUCTION_COUNT];

uint8_t cycles = 0;
size_t totalCycles = 0;

uint8_t currentOpcode = 0;
uint8_t operand = 0;
uint16_t effectiveAddress = 0;

size_t instructionCount = 0;

static inline void push(uint8_t data) {
	bus_write(0x0100 | registers.SP--, data);
}

static inline uint8_t pull() {
	return bus_read(0x0100 | ++registers.SP);
}

static inline void branch(bool condition) {
	if (condition) {
		cycles++;
		registers.PC = effectiveAddress;
	}
}

void cpu_irq() {
	if (!registers.I)
		return;

	push(registers.PC >> 8);
	push(registers.PC & 0xFF);
	push(registers.flags);

	registers.PC_LO = bus_read(0xFFFE);
	registers.PC_HI = bus_read(0xFFFF);

	registers.I = true;
#ifdef WDC
	registers.D = 0;
#endif

	cycles = 7;
}

void cpu_reset() {
	registers.PC_LO = bus_read(0xFFFC);
	registers.PC_HI = bus_read(0xFFFD);

	registers.SP = (uint8_t) ((rand() / (float) RAND_MAX) * 0xFF);

	registers._ = 1;
	registers.B = 1;
#ifdef WDC
	registers.D = 0;
#endif
	registers.I = 1;

	cycles = 7;

	// reset internal state
	currentOpcode = 0;
	operand = 0;
	effectiveAddress = 0;
	instructionCount = 0;
	totalCycles = 0;
}

void cpu_nmi() {
	push(registers.PC >> 8);
	push(registers.PC & 0xFF);
	push(registers.flags);

	registers.PC_LO = bus_read(0xFFFA);
	registers.PC_HI = bus_read(0xFFFB);

	registers.I = true;
#ifdef WDC
	registers.D = 0;
#endif

	cycles = 7;
}

// run single instruction
// waits to run the instruction until no more cycles need to be consumed
// afterwards runs entire instruction in a single clockcycle
// if a cycle needs to be consumed, this function returns early and consumes a single cycle
void cpu_clock() {
	if (cycles-- != 0)
		return;

	instructionCount++;

	currentOpcode = bus_read(registers.PC++);

	const struct opcode opcode = opcodes[currentOpcode];

#ifdef VERBOSE
	printf("$%04X: %-*s %s (", registers.PC - 1, INSTRUCTION_NAME_LENGTH, instructions[opcode.instruction].name, addressModes[opcode.addressMode].name);

	printf("%-*s ", INSTRUCTION_NAME_LENGTH, instructions[opcode.instruction].name);
	switch(opcode.addressMode) {
#ifndef WDC
		case AM_XXX: printf("         "); break;
#endif
		case AM_IMP: printf("         "); break;
		case AM_ACC: printf("A        "); break;
#ifdef WDC
		case AM_STK: printf("         "); break;
#endif
		case AM_REL: printf("%c$%02X     ", (int8_t) bus_read(registers.PC) < 0 ? '-' : bus_read(registers.PC) == 0 ? ' ' : '+', abs((int8_t) bus_read(registers.PC))); break;
		case AM_IMM: printf("#$%02X     ", bus_read(registers.PC)); break;
		case AM_ABS: printf("$%02X%02X    ", bus_read(registers.PC + 1), bus_read(registers.PC)); break;
#ifdef WDC
		case AM_ABSI: printf("($%02X%02X,X)", bus_read(registers.PC + 1), bus_read(registers.PC)); break;
#endif
		case AM_ABSX: printf("$%02X%02X,X  ", bus_read(registers.PC + 1), bus_read(registers.PC)); break;
		case AM_ABSY: printf("$%02X%02X,Y  ", bus_read(registers.PC + 1), bus_read(registers.PC)); break;
		case AM_ZPG: printf("$%02X      ", bus_read(registers.PC)); break;
#ifdef WDC
		case AM_ZPGI: printf("($%02X)    ", bus_read(registers.PC)); break;
#endif
		case AM_ZPGX: printf("$%02X,X      ", bus_read(registers.PC)); break;
		case AM_ZPGY: printf("$%02X,Y      ", bus_read(registers.PC)); break;
		case AM_IND: printf("($%02X%02X)  ", bus_read(registers.PC + 1), bus_read(registers.PC)); break;
		case AM_INDX: printf("($%02X%02X,X)", bus_read(registers.PC + 1), bus_read(registers.PC)); break;
		case AM_INDY: printf("($%02X%02X),Y", bus_read(registers.PC + 1), bus_read(registers.PC)); break;
	}
	printf(")\n");
#endif

	// set cycles from instruction
	cycles = opcode.cycleCount;

	addressModes[opcode.addressMode].func();
	instructions[opcode.instruction].func();

	totalCycles += cycles;
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

#ifdef __GNUC__
static inline void printBin(uint8_t val) {
	printf("%08b", val);
}
#else
static inline void printBin(uint8_t val) {
	for (int i = 7; i >= 0; i--)
		printf("%c", (val & (1 << i)) ? '1' : '0');
}
#endif

void cpu_printRegisters() {
	printf("=------=----=----=----=----------=----=\n");
	printf("|  PC  |  A |  X |  Y | NV_BDIZC | SP |\n");
	printf("| %04X | %02X | %02X | %02X | ", registers.PC, registers.A, registers.X, registers.Y);
	printBin(registers.flags);
	printf(" | %02X |\n", registers.SP);
	printf("=------=----=----=----=----------=----=\n");
}

#pragma region addressingModes
// the following functions all implement logic for the addressing mode


// absolute addressing mode
// an absolute memory location is provided
// the value at this memory location is used as operand
// in case of a jump instruction the memory location provided is the address to jump to
void am_abs() {
	effectiveAddress = bus_read(registers.PC++);
	effectiveAddress |= (bus_read(registers.PC++) << 8);
	operand = bus_read(effectiveAddress);
}

#ifdef WDC
// absolute indexed indirect addressing mode
// an absolute memory location is provided
// this memory location is incremented by X, this memory location provides 2 bytes to actually use, in the format $LLHH
// this mode is only used for JMP
void am_absi() {
	uint16_t addr = bus_read(registers.PC++);
	addr |= (bus_read(registers.PC++) << 8);
	addr += registers.X;
	effectiveAddress = bus_read(addr) | (bus_read(addr + 1) << 8);
}
#endif

// absolute addressing mode offset by X
// an absolute memory location is provided
// this memory location is incremented by X, and the value at that memory location is used as operand
// this can be used to loop through a set of data, aka an array
void am_absx() {
	effectiveAddress = bus_read(registers.PC++);
	effectiveAddress |= (bus_read(registers.PC++) << 8);
	effectiveAddress += registers.X;
}

// absolute addressing mode offset by Y
// an absolute memory location is provided
// this memory location is incremented by Y, and the value at that memory location is used as operand
// this can be used to loop through a set of data, aka an array
void am_absy() {
	effectiveAddress = bus_read(registers.PC++);
	effectiveAddress |= (bus_read(registers.PC++) << 8);
	effectiveAddress += registers.Y;
}

// immediate addressing mode
// operand is provided directly after the instruction
void am_imm() {
	operand = bus_read(registers.PC++);
}

// implied addressing mode
// operand is implied by the instruction
//   this also includes accumulator addressing mode, as accumulator is the implied operand
//   this also includes stack addressing mode documented in the WDC data sheets, as stack pointer is the implied operand
void am_imp() {
	// no need to do anything, instruction will not use operand/effectiveAddress anyways
}

// indirect addressing mode
// an absolute memory location is provided
// this memory location provides 2 byte to actually use, in the format $LLHH
// this mode is generally only used for JMP
void am_ind() {
	uint16_t addr = bus_read(registers.PC++);
	addr |= (bus_read(registers.PC++) << 8);
	effectiveAddress = bus_read(addr) | (bus_read(addr + 1) << 8);
}

// pre-indexed indirect addressing mode
// a byte is provided, which describes the offset in the zero-page
// from this zero-page address two bytes are read ($LLHH), which gets incremented by X
// this increased memory address points to the memory address ($LLHH) where the actual data is stored
void am_indx() {
	uint8_t offset = bus_read(registers.PC++);
	offset += registers.X;
	uint16_t addr = bus_read(offset) | (bus_read(offset + 1) << 8);
	operand = bus_read(addr);
}

// post-indexed indirect addressing mode
// a byte is provided, which describes the offset in the zero-page
// from this zero-page address two bytes are read ($LLHH)
// this memory address points to the memory address ($LLHH), which gets incremented with Y
// this address is where the actual data is stored
void am_indy() {
	uint8_t offset = bus_read(registers.PC++);
	uint16_t addr = bus_read(offset) | (bus_read(offset + 1) << 8);
	addr += registers.Y;
	operand = bus_read(addr);
}

// relative addressing mode
// operand provided is a signed byte
// this byte is added to PC to get the address to branch to
// this mode is only allowed for the branch instructions
void am_rel() {
	int8_t offset = bus_read(registers.PC++);
	effectiveAddress = registers.PC + offset;
}

// zero-page addressing mode
// a byte is provided directly after the instruction which contains an offset in the zero-page
// the absolute address would be $00XX
// this makes the operation faster, and shorter
// the instruction takes only 2 bytes, instead of 3 for a full address
void am_zpg() {
	operand = bus_read(registers.PC++);
}

#ifdef WDC
// zero-page indirect addressing mode
// a byte is provided directly after the instruction which contains an offset in the zero-page
// this memory location provides 2 byte to actually use, in the format $LLHH
void am_zpgi() {
	uint8_t offset = bus_read(registers.PC++);
	effectiveAddress = bus_read(offset) | (bus_read(offset + 1) << 8);
}
#endif

// zero-page addressing mode offset by X
// a byte is provided directly after the instruction which contains an offset in the zero-page
// this memory location is incremented by X, and the value at that memory location is used as operand
// this can be used to loop through a set of data, aka an array
// this makes the operation faster, and shorter
// the instruction takes only 2 bytes, instead of 3 for a full address
// the result of the addition wraps around, so the byte read will always be inside the zero-page
void am_zpgx() {
	uint8_t offset = bus_read(registers.PC++);
	offset += registers.X;
	operand = bus_read(offset);
}

// zero-page addressing mode offset by Y
// a byte is provided directly after the instruction which contains an offset in the zero-page
// this memory location is incremented by Y, and the value at that memory location is used as operand
// this can be used to loop through a set of data, aka an array
// this makes the operation faster, and shorter
// the instruction takes only 2 bytes, instead of 3 for a full address
// the result of the addition wraps around, so the byte read will always be inside the zero-page
// this addressing mode is only used if the register used is X (LDX, STX), so X cannot be used
void am_zpgy() {
	uint8_t offset = bus_read(registers.PC++);
	offset += registers.Y;
	operand = bus_read(offset);
}

#ifndef WDC
// addressing mode for illegal opcodes
// shouldn't be used in normal programs
//   the 6502 has a couple of opcodes that gave somewhat reliable results
//   the WDC version of the 6502 has all illegal opcodes as implemented as nops
void am_xxx() {
#ifdef VERBOSE
	printf("ILLEGAL ADDRESS MODE EXECUTED\n");
#endif
}
#endif

#pragma endregion addressingModes

#pragma region instructions
// the following functions all implement logic for the instructions

#ifdef ROCKWEL
// some macro magic to get an easier time implementing the bit instructions
// these instrutions originated in the Rockwel chips, and were later adapted by WDC
#define BITS_EXPANSION(funcName) \
void in_##funcName(uint8_t bit); \
BIT_EXPANSION(funcName, 0) BIT_EXPANSION(funcName, 1) \
BIT_EXPANSION(funcName, 2) BIT_EXPANSION(funcName, 3) BIT_EXPANSION(funcName, 4) \
BIT_EXPANSION(funcName, 5) BIT_EXPANSION(funcName, 6) BIT_EXPANSION(funcName, 7)
#define BIT_EXPANSION(funcName, bit) \
void in_##funcName##bit() { in_##funcName(bit); }
#endif

// ADd with Carry
// adds operand to accumulator with carry flag
void in_adc() {
	uint16_t tmp = operand + registers.A + registers.C;

	if (registers.D) {
		if ((tmp & 0x0F) >= 0x0A)
			tmp += 0x06;
		if ((tmp & 0xF0) >= 0xA0)
			tmp += 0x60;
	}

	registers.V = ((registers.A & 0x80) == (operand & 0x80)) && ((registers.A & 0x80) != (tmp & 0x80));

	registers.C = tmp > 0xFF;
	registers.A = tmp & 0xFF;

	registers.Z = registers.A == 0;
	registers.N = registers.A & 0x80;
}

// bitwise AND
// performs a bitwise and with operand and accumulator
void in_and() {
	registers.A &= operand;

	registers.Z = registers.A == 0;
	registers.N = registers.A & 0x80;
}

// Arithmatic Shift Left
// shifts 0 into bit 0
// shifts bit 7 into carry flag
void in_asl() {
	NO_IMPL();
}

#ifdef ROCKWEL
// Branch on Bit Reset
// tests bit of accumulator, and branches if it is 0
// a branch taken takes an extra clock cycle
void in_bbr(uint8_t bit) {
	branch((registers.A & (1 << bit)) == 0);
}

BITS_EXPANSION(bbr)
#endif

#ifdef ROCKWEL
// Branch on Bit Set
// tests bit of accumulator, and branches if it is 1
// a branch taken takes an extra clock cycle
void in_bbs(uint8_t bit) {
	branch((registers.A & (1 << bit)) == 1);
}

BITS_EXPANSION(bbs)
#endif

// Branch Carry Clear
// branches when carry flag is unset
// a branch taken takes an extra clock cycle
void in_bcc() {
	branch(!registers.C);
}

// Branch Carry Set
// branches when carry flag is set
// a branch taken takes an extra clock cycle
void in_bcs() {
	branch(registers.C);
}

// Branch on EQual
// branches when zero flag is set (two values are equal if A - B == 0)
// a branch taken takes an extra clock cycle
void in_beq() {
	branch(registers.Z);
}

// test BITs
// probably the most confusing instruction of the 6502
// zero flag is set according to the operation accumulator AND operand
// bits 6 and 7 of operand are set as negative and overflow flags respectively
// this instruction only alters flags register
void in_bit() {
	registers.Z = (registers.A & operand) == 0;
	registers.flags |= operand & 0xC0;
}

// Branch on MInus
// branches when negative flag is set
// a branch taken takes an extra clock cycle
void in_bmi() {
	branch(registers.N);
}

// Branch on Not Equals
// branches when zero flag is unset (two values are not equal if A - B != 0)
// a branch taken takes an extra clock cycle
void in_bne() {
	branch(!registers.Z);
}

// Branch on PLus
// branches when negative flag is unset
// a branch taken takes an extra clock cycle
void in_bpl() {
	branch(!registers.N);
}

#ifdef WDC
// Branch Always
// a branch taken takes an extra clock cycle
//   this is always the case, so don't take an extra clock cycle, it is added in the opcode table
void in_bra() {
	cycles--; // correct the cycles increment from branch
	branch(true);
}
#endif

// BReaK
// forces an interupt to occur
// this starts the IRQ sequence with break flag set
// the return address is PC + 2, making the byte following this instruction being skipped
// this byte can be used as a break mark
void in_brk() {
	NO_IMPL();
}

// Branch on oVerflow Clear
// branches when overflow flag is unset
// a branch taken takes an extra clock cycle
void in_bvc() {
	branch(!registers.V);
}

// Branch on oVerflow Set
// branches when overflow flag is set
// a branch taken takes an extra clock cycle
void in_bvs() {
	branch(registers.V);
}

// CLear Carry flag
void in_clc() {
	registers.C = false;
}

// CLear Decimal flag
// makes the 6502 perform normal binary math
void in_cld() {
	registers.D = false;
}

// CLear Interupt flag
// this instruction enables interupts from the IRQ pin/function, as this pin is active low
void in_cli() {
	registers.I = false;
}

// CLear oVerflow
void in_clv() {
	registers.V = false;
}

// CoMPare with accumulator
// subtract operand with accumulator, and set flags register accordingly
// then ignore the result from the subtraction
// this instruction only alters flags register
void in_cmp() {
	registers.Z = registers.A == operand;
	registers.C = registers.A >= operand;
	if (registers.A == operand)
		registers.N = 0;
	else
		registers.N = (int8_t)(registers.A - operand) < 0;
}

// CoMpare with X register
// subtract operand with accumulator, and set flags register accordingly
// then ignore the result from the subtraction
// this instruction only alters flags register
void in_cpx() {
	registers.Z = registers.X == operand;
	registers.C = registers.X >= operand;
	if (registers.X == operand)
		registers.N = 0;
	else
		registers.N = (int8_t)(registers.X - operand) < 0;
}

// CoMpare with Y register
// subtract operand with accumulator, and set flags register accordingly
// then ignore the result from the subtraction
// this instruction only alters flags register
void in_cpy() {
	registers.Z = registers.Y == operand;
	registers.C = registers.Y >= operand;
	if (registers.Y == operand)
		registers.N = 0;
	else
		registers.N = (int8_t)(registers.Y - operand) < 0;
}

// DECrement operand
void in_dec() {
	operand--;
	registers.Z = operand == 0;
	registers.N = operand & 0x80;
	bus_write(effectiveAddress, operand);
}

// DEcrement X register
void in_dex() {
	registers.X--;
	registers.Z = registers.X == 0;
	registers.N = registers.X & 0x80;
}

// DEcrement Y register
void in_dey() {
	registers.Y--;
	registers.Z = registers.Y == 0;
	registers.N = registers.Y & 0x80;
}

// bitwise eXclusive OR
// performs a bitwise exclusive or with operand and accumulator
void in_eor() {
	registers.A ^= operand;

	registers.Z = registers.A == 0;
	registers.N = registers.A & 0x80;
}

// INCrement operand
void in_inc() {
	operand++;
	registers.Z = operand == 0;
	registers.N = operand & 0x80;
	bus_write(effectiveAddress, operand);
}

// INcrement X register
void in_inx() {
	registers.X++;
	registers.Z = registers.X == 0;
	registers.N = registers.X & 0x80;
}

// INcrement Y register
void in_iny() {
	registers.Y++;
	registers.Z = registers.Y == 0;
	registers.N = registers.Y & 0x80;
}

// JuMP
// loads PC with operand
void in_jmp() {
	registers.PC = effectiveAddress;
}

// Jump to SubRoutine
// saves current PC in stack
// loads PC with operand
void in_jsr() {
	registers.PC--;
	push(registers.PC_HI);
	push(registers.PC_LO);
	registers.PC = effectiveAddress;
}

// LoaD Accumulator with operand
void in_lda() {
	registers.A = operand;
	registers.Z = registers.A == 0;
	registers.N = registers.A & 0x80;
}

// LoaD X register with operand
void in_ldx() {
	registers.X = operand;
	registers.Z = registers.X == 0;
	registers.N = registers.X & 0x80;
}

// LoaD Y register with operand
void in_ldy() {
	registers.Y = operand;
	registers.Z = registers.Y == 0;
	registers.N = registers.Y & 0x80;
}

// Logical Shift Right
// shifts 0 into bit 7
// shifts bit 0 into carry flag
void in_lsr() {
	NO_IMPL();
}

// No OPeration
// this instruction does nothing
//   the WDC version of the 6502 has all illegal opcodes as implemented as nops
//   these differ slightly in operand size and/or cycle counts
void in_nop() {
#ifdef WDC
	uint8_t nop2[] = {
		0x02, 0x22, 0x42, 0x62, 0x82, 0xC2, 0xE2,	// 2 cycles
		0x44,										// 3 cycles
		0x54, 0xD4, 0xF4							// 4 cycles
	};
	uint8_t nop3[] = {
		0xDC, 0xFC,									// 4 cycles
		0x5C										// 8 cycles
	};

	for (size_t i = 0; i < sizeof(nop2) / sizeof(nop2[0]); i++)
		if (currentOpcode == nop2[i])
			registers.PC++;
	for (size_t i = 0; i < sizeof(nop3) / sizeof(nop3[0]); i++)
		if (currentOpcode == nop3[i])
			registers.PC += 2;
#endif
}

// bitwise OR with Accumulator
// performs a bitwise or with operand and accumulator
void in_ora() {
	registers.A |= operand;

	registers.Z = registers.A == 0;
	registers.N = registers.A & 0x80;
}

// PusH Accumulator on stack
void in_pha() {
	push(registers.A);
}

// PusH Processor status
// pushes flags register on stack
// this instruction sets break flag and bit 5 (unused)
void in_php() {
	push(registers.flags | 0x30);
}

#ifdef WDC
// PusH X register on stack
void in_phx() {
	push(registers.X);
}
#endif

#ifdef WDC
// PusH Y register on stack
void in_phy() {
	push(registers.Y);
}
#endif

// PuLl Accumulator off stack
void in_pla() {
	registers.A = pull();
	registers.Z = registers.A == 0;
	registers.N = registers.A & 0x80;
}

// PuLl Processor status
// pulls flags register off stack
// this instruction ignores break flag and bit 5 (unused)
void in_plp() {
	registers.flags = pull() & 0xCF;
}

#ifdef WDC
// PuLl X register off stack
void in_plx() {
	registers.X = pull();
	registers.Z = registers.X == 0;
	registers.N = registers.X & 0x80;
}
#endif

#ifdef WDC
// PuLl Y register off stack
void in_ply() {
	registers.Y = pull();
	registers.Z = registers.Y == 0;
	registers.N = registers.Y & 0x80;
}
#endif

#ifdef ROCKWEL
// Reset Memory Bit
// sets bit at operand to 0
void in_rmb(uint8_t bit) {
	(void) bit;
	NO_IMPL();
}

BITS_EXPANSION(rmb)
#endif

// ROtate Left
// shifts carry flag into bit 0
// shifts bit 7 into carry flag
void in_rol() {
	NO_IMPL();
}

// ROtate Right
// shifts carry flag into bit 7
// shifts bit 0 into carry flag
//   early versions of the 6502 have a bug in this instruction
//   the instruction would instead perform as an ASL, without shifting bit 7 into carry flag
//   this bug makes it so that carry flag would be unused during the instruction
void in_ror() {
	NO_IMPL();
}

// ReTurn from Interupt
// pulls flags register from stack, ignoring break flag and bit 5 (ignored)
// then pulls PC from stack
void in_rti() {
	NO_IMPL();
}

// ReTurn from Subroutine
// pulls PC from stack
void in_rts() {
	NO_IMPL();
}

// SuBtract with Carry
// subtract operand from accumulator with carry flag as a borrow
// carry flag should be set before being called
// if carry flag is cleared after the call, a borrow was needed
void in_sbc() {
	NO_IMPL();
}

// SEt Carry flag
void in_sec() {
	registers.C = true;
}

// SEt Decimal flag
// makes the 6502 perform binary coded decimal math
void in_sed() {
	registers.D = true;
}

// SEt Interupt flag
// this instruction disables interupts from the IRQ pin/function, as this pin is active low
void in_sei() {
	registers.I = true;
}

#ifdef ROCKWEL
// Set Memory Bit
// sets bit at operand to 1
void in_smb(uint8_t bit) {
	(void) bit;
	NO_IMPL();
}

BITS_EXPANSION(smb)
#endif

// STore Accumulator at operand
void in_sta() {
	bus_write(effectiveAddress, registers.A);
}

#ifdef WDC
// SToP
// stops the clock from affecting the 65c02
// the 65c02 is faster to respond to the reset pin/function
void in_stp() {
	NO_IMPL();
}
#endif

// STore X register at operand
void in_stx() {
	bus_write(effectiveAddress, registers.X);
}

// STore Y register at operand
void in_sty() {
	bus_write(effectiveAddress, registers.Y);
}

#ifdef WDC
// STore Zero at operand
void in_stz() {
	bus_write(effectiveAddress, 0);
}
#endif

// Transfer Accumulator to X register
void in_tax() {
	registers.X = registers.A;
	registers.Z = registers.X == 0;
	registers.N = registers.X & 0x80;
}

// Transfer Accumulator to Y register
void in_tay() {
	registers.Y = registers.A;
	registers.Z = registers.Y == 0;
	registers.N = registers.Y & 0x80;
}

#ifdef WDC
// Test and Reset Bit
// clears bits set in accumulator at operand
// sets zero flag if any bits were changed, otherwise it gets cleared
void in_trb() {
	NO_IMPL();
}
#endif

#ifdef WDC
// Test and Set Bit
// sets bits set in accumulator at operand
// sets zero flag if any bits were changed, otherwise it gets cleared
void in_tsb() {
	NO_IMPL();
}
#endif

// Transfer Stack pointer to X register
void in_tsx() {
	registers.X = registers.SP;
	registers.Z = registers.X == 0;
	registers.N = registers.X & 0x80;
}

// Transfer X register to Accumulator
void in_txa() {
	registers.A = registers.X;
	registers.Z = registers.A == 0;
	registers.N = registers.A & 0x80;
}

// Transfer X register to Stack pointer
void in_txs() {
	registers.SP = registers.X;
}

// Transfer Y register to Accumulator
void in_tya() {
	registers.A = registers.Y;
	registers.Z = registers.A == 0;
	registers.N = registers.A & 0x80;
}

#ifdef WDC
// WAit for Interupt
// stops the clock from affecting the 65c02
// the 65c02 is faster to respond to the IRQ/NMI pins/functions
void in_wai() {
	NO_IMPL();
}
#endif

#ifndef WDC
// instruction for illegal opcodes
// shouldn't be used in normal programs
//   the 6502 has a couple of opcodes that gave somewhat reliable results
//   the WDC version of the 6502 has all illegal opcodes as implemented as nops
void in_xxx() {
}
#endif

#ifdef ROCKWEL
#undef BITS_EXPANSION
#undef BIT_EXPANSION
#endif

#pragma endregion instructions

#pragma region data

#if defined(WDC)
struct opcode opcodes[] = {
	{IN_BRK ,AM_STK ,7},{IN_ORA ,AM_INDX,6},{IN_NOP ,AM_IMP ,2},{IN_NOP ,AM_IMP ,1},{IN_TSB ,AM_ZPG ,5},{IN_ORA ,AM_ZPG ,3},{IN_ASL ,AM_ZPG ,5},{IN_RMB0,AM_ZPG ,5},{IN_PHP ,AM_STK ,3},{IN_ORA ,AM_IMM ,2},{IN_ASL ,AM_ACC ,2},{IN_NOP ,AM_IMP ,1},{IN_TSB ,AM_ABS ,6},{IN_ORA ,AM_ABS ,4},{IN_ASL ,AM_ABS ,6},{IN_BBR0,AM_REL ,5},
	{IN_BPL ,AM_REL ,2},{IN_ORA ,AM_INDY,5},{IN_ORA ,AM_ZPG ,5},{IN_NOP ,AM_IMP ,1},{IN_TRB ,AM_ZPG ,5},{IN_ORA ,AM_ZPGX,4},{IN_ASL ,AM_ZPGX,6},{IN_RMB1,AM_ZPG ,5},{IN_CLC ,AM_IMP ,2},{IN_ORA ,AM_ABSY,4},{IN_INC ,AM_ACC ,2},{IN_NOP ,AM_IMP ,1},{IN_TRB ,AM_ABS ,6},{IN_ORA ,AM_ABSX,4},{IN_ASL ,AM_ABSX,7},{IN_BBR1,AM_REL ,5},
	{IN_JSR ,AM_ABS ,6},{IN_AND ,AM_INDX,6},{IN_NOP ,AM_IMP ,2},{IN_NOP ,AM_IMP ,1},{IN_BIT ,AM_ZPG ,3},{IN_AND ,AM_ZPG ,3},{IN_ROL ,AM_ZPG ,5},{IN_RMB2,AM_ZPG ,5},{IN_PLP ,AM_STK ,4},{IN_AND ,AM_IMM ,2},{IN_ROL ,AM_ACC ,2},{IN_NOP ,AM_IMP ,1},{IN_BIT ,AM_ABS ,4},{IN_AND ,AM_ABS ,4},{IN_ROL ,AM_ABS ,6},{IN_BBR2,AM_REL ,5},
	{IN_BMI ,AM_REL ,2},{IN_AND ,AM_INDY,5},{IN_AND ,AM_ZPG ,5},{IN_NOP ,AM_IMP ,1},{IN_BIT ,AM_ZPGX,4},{IN_AND ,AM_ZPGX,4},{IN_ROL ,AM_ZPGX,6},{IN_RMB3,AM_ZPG ,5},{IN_SEC ,AM_IMP ,2},{IN_AND ,AM_ABSY,4},{IN_DEC ,AM_ACC ,2},{IN_NOP ,AM_IMP ,1},{IN_BIT ,AM_ABSX,4},{IN_AND ,AM_ABSX,4},{IN_ROL ,AM_ABSX,7},{IN_BBR3,AM_REL ,5},
	{IN_RTI ,AM_STK ,6},{IN_EOR ,AM_INDX,6},{IN_NOP ,AM_IMP ,2},{IN_NOP ,AM_IMP ,1},{IN_NOP ,AM_IMP ,3},{IN_EOR ,AM_ZPG ,3},{IN_LSR ,AM_ZPG ,5},{IN_RMB4,AM_ZPG ,5},{IN_PHA ,AM_STK ,3},{IN_EOR ,AM_IMM ,2},{IN_LSR ,AM_ACC ,2},{IN_NOP ,AM_IMP ,1},{IN_JMP ,AM_ABS ,3},{IN_EOR ,AM_ABS ,4},{IN_LSR ,AM_ABS ,6},{IN_BBR4,AM_REL ,5},
	{IN_BVC ,AM_REL ,2},{IN_EOR ,AM_INDY,5},{IN_EOR ,AM_ZPG ,5},{IN_NOP ,AM_IMP ,1},{IN_NOP ,AM_IMP ,4},{IN_EOR ,AM_ZPGX,4},{IN_LSR ,AM_ZPGX,6},{IN_RMB5,AM_ZPG ,5},{IN_CLI ,AM_IMP ,2},{IN_EOR ,AM_ABSY,4},{IN_PHY ,AM_STK ,3},{IN_NOP ,AM_IMP ,1},{IN_NOP ,AM_IMP ,8},{IN_EOR ,AM_ABSX,4},{IN_LSR ,AM_ABSX,7},{IN_BBR5,AM_REL ,5},
	{IN_RTS ,AM_STK ,6},{IN_ADC ,AM_INDX,6},{IN_NOP ,AM_IMP ,2},{IN_NOP ,AM_IMP ,1},{IN_STZ ,AM_ZPG ,3},{IN_ADC ,AM_ZPG ,3},{IN_ROR ,AM_ZPG ,5},{IN_RMB6,AM_ZPG ,5},{IN_PLA ,AM_STK ,4},{IN_ADC ,AM_IMM ,2},{IN_ROR ,AM_ACC ,2},{IN_NOP ,AM_IMP ,1},{IN_JMP ,AM_IND ,5},{IN_ADC ,AM_ABS ,4},{IN_ROR ,AM_ABS ,6},{IN_BBR6,AM_REL ,5},
	{IN_BVS ,AM_REL ,2},{IN_ADC ,AM_INDY,5},{IN_ADC ,AM_ZPG ,5},{IN_NOP ,AM_IMP ,1},{IN_STZ ,AM_ZPGX,4},{IN_ADC ,AM_ZPGX,4},{IN_ROR ,AM_ZPGX,6},{IN_RMB7,AM_ZPG ,5},{IN_SEI ,AM_IMP ,2},{IN_ADC ,AM_ABSY,4},{IN_PLY ,AM_STK ,4},{IN_NOP ,AM_IMP ,1},{IN_JMP ,AM_ABSI,6},{IN_ADC ,AM_ABSX,4},{IN_ROR ,AM_ABSX,7},{IN_BBR7,AM_REL ,5},

	{IN_BRA ,AM_REL ,3},{IN_STA ,AM_INDX,6},{IN_NOP ,AM_IMP ,2},{IN_NOP ,AM_IMP ,1},{IN_STY ,AM_ZPG ,3},{IN_STA ,AM_ZPG ,3},{IN_STX ,AM_ZPG ,3},{IN_SMB0,AM_ZPG ,5},{IN_DEY ,AM_IMP ,2},{IN_BIT ,AM_IMM ,2},{IN_TXA ,AM_IMP ,2},{IN_NOP ,AM_IMP ,1},{IN_STY ,AM_ABS ,4},{IN_STA ,AM_ABS ,4},{IN_STX ,AM_ABS ,4},{IN_BBS0,AM_REL ,5},
	{IN_BCC ,AM_REL ,2},{IN_STA ,AM_INDY,6},{IN_STA ,AM_ZPG ,5},{IN_NOP ,AM_IMP ,1},{IN_STY ,AM_ZPGX,4},{IN_STA ,AM_ZPGX,4},{IN_STX ,AM_ZPGY,4},{IN_SMB1,AM_ZPG ,5},{IN_TYA ,AM_IMP ,2},{IN_STA ,AM_ABSY,5},{IN_TXS ,AM_IMP ,2},{IN_NOP ,AM_IMP ,1},{IN_STZ ,AM_ABS ,4},{IN_STA ,AM_ABSX,5},{IN_STZ ,AM_ABSX,5},{IN_BBS1,AM_REL ,5},
	{IN_LDY ,AM_IMM ,2},{IN_LDA ,AM_INDX,6},{IN_LDX ,AM_IMM ,2},{IN_NOP ,AM_IMP ,1},{IN_LDY ,AM_ZPG ,3},{IN_LDA ,AM_ZPG ,3},{IN_LDX ,AM_ZPG ,3},{IN_SMB2,AM_ZPG ,5},{IN_TAY ,AM_IMP ,2},{IN_LDA ,AM_IMM ,2},{IN_TAX ,AM_IMP ,2},{IN_NOP ,AM_IMP ,1},{IN_LDY ,AM_ABS ,4},{IN_LDA ,AM_ABS ,4},{IN_LDX ,AM_ABS ,4},{IN_BBS2,AM_REL ,5},
	{IN_BCS ,AM_REL ,2},{IN_LDA ,AM_INDY,5},{IN_LDA ,AM_ZPG ,5},{IN_NOP ,AM_IMP ,1},{IN_LDY ,AM_ZPGX,4},{IN_LDA ,AM_ZPGX,4},{IN_LDX ,AM_ZPGY,4},{IN_SMB3,AM_ZPG ,5},{IN_CLV ,AM_IMP ,2},{IN_LDA ,AM_ABSY,4},{IN_TSX ,AM_IMP ,2},{IN_NOP ,AM_IMP ,1},{IN_LDY ,AM_ABSX,4},{IN_LDA ,AM_ABSX,4},{IN_LDX ,AM_ABSY,4},{IN_BBS3,AM_REL ,5},
	{IN_CPY ,AM_IMM ,2},{IN_CMP ,AM_INDX,6},{IN_NOP ,AM_IMP ,2},{IN_NOP ,AM_IMP ,1},{IN_CPY ,AM_ZPG ,3},{IN_CMP ,AM_ZPG ,3},{IN_DEC ,AM_ZPG ,5},{IN_SMB4,AM_ZPG ,5},{IN_INY ,AM_IMP ,2},{IN_CMP ,AM_IMM ,2},{IN_DEX ,AM_IMP ,2},{IN_WAI ,AM_IMP ,3},{IN_CPY ,AM_ABS ,4},{IN_CMP ,AM_ABS ,4},{IN_DEC ,AM_ABS ,6},{IN_BBS4,AM_REL ,5},
	{IN_BNE ,AM_REL ,2},{IN_CMP ,AM_INDY,5},{IN_CMP ,AM_ZPG ,5},{IN_NOP ,AM_IMP ,1},{IN_NOP ,AM_IMP ,4},{IN_CMP ,AM_ZPGX,4},{IN_DEC ,AM_ZPGX,6},{IN_SMB5,AM_ZPG ,5},{IN_CLD ,AM_IMP ,2},{IN_CMP ,AM_ABSY,4},{IN_PHX ,AM_STK ,3},{IN_STP ,AM_IMP ,3},{IN_NOP ,AM_IMP ,4},{IN_CMP ,AM_ABSX,4},{IN_DEC ,AM_ABSX,7},{IN_BBS5,AM_REL ,5},
	{IN_CPX ,AM_IMM ,2},{IN_SBC ,AM_INDX,6},{IN_NOP ,AM_IMP ,2},{IN_NOP ,AM_IMP ,1},{IN_CPX ,AM_ZPG ,3},{IN_SBC ,AM_ZPG ,3},{IN_INC ,AM_ZPG ,5},{IN_SMB6,AM_ZPG ,5},{IN_INX ,AM_IMP ,2},{IN_SBC ,AM_IMM ,2},{IN_NOP ,AM_IMP ,2},{IN_NOP ,AM_IMP ,1},{IN_CPX ,AM_ABS ,4},{IN_SBC ,AM_ABS ,4},{IN_INC ,AM_ABS ,6},{IN_BBS6,AM_REL ,5},
	{IN_BEQ ,AM_REL ,2},{IN_SBC ,AM_INDY,5},{IN_SBC ,AM_ZPG ,5},{IN_NOP ,AM_IMP ,1},{IN_NOP ,AM_IMP ,4},{IN_SBC ,AM_ZPGX,4},{IN_INC ,AM_ZPGX,6},{IN_SMB7,AM_ZPG ,5},{IN_SED ,AM_IMP ,2},{IN_SBC ,AM_ABSY,4},{IN_PLX ,AM_STK ,4},{IN_NOP ,AM_IMP ,1},{IN_NOP ,AM_IMP ,4},{IN_SBC ,AM_ABSX,4},{IN_INC ,AM_ABSX,7},{IN_BBS7,AM_REL ,5},
};
#elif defined(ROCKWEL)
struct opcode opcodes[] = {
	{IN_BRK ,AM_IMP ,7},{IN_ORA ,AM_INDX,6},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_ORA ,AM_ZPG ,3},{IN_ASL ,AM_ZPG ,5},{IN_RMB0,AM_ZPG ,5},{IN_PHP ,AM_IMP ,3},{IN_ORA ,AM_IMM ,2},{IN_ASL ,AM_ACC ,2},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_ORA ,AM_ABS ,4},{IN_ASL ,AM_ABS ,6},{IN_BBR0,AM_REL ,5},
	{IN_BPL ,AM_REL ,2},{IN_ORA ,AM_INDY,5},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_ORA ,AM_ZPGX,4},{IN_ASL ,AM_ZPGX,6},{IN_RMB1,AM_ZPG ,5},{IN_CLC ,AM_IMP ,2},{IN_ORA ,AM_ABSY,4},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_ORA ,AM_ABSX,4},{IN_ASL ,AM_ABSX,7},{IN_BBR1,AM_REL ,5},
	{IN_JSR ,AM_ABS ,6},{IN_AND ,AM_INDX,6},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_BIT ,AM_ZPG ,3},{IN_AND ,AM_ZPG ,3},{IN_ROL ,AM_ZPG ,5},{IN_RMB2,AM_ZPG ,5},{IN_PLP ,AM_IMP ,4},{IN_AND ,AM_IMM ,2},{IN_ROL ,AM_ACC ,2},{IN_XXX ,AM_XXX ,0},{IN_BIT ,AM_ABS ,4},{IN_AND ,AM_ABS ,4},{IN_ROL ,AM_ABS ,6},{IN_BBR2,AM_REL ,5},
	{IN_BMI ,AM_REL ,2},{IN_AND ,AM_INDY,5},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_AND ,AM_ZPGX,4},{IN_ROL ,AM_ZPGX,6},{IN_RMB3,AM_ZPG ,5},{IN_SEC ,AM_IMP ,2},{IN_AND ,AM_ABSY,4},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_AND ,AM_ABSX,4},{IN_ROL ,AM_ABSX,7},{IN_BBR3,AM_REL ,5},
	{IN_RTI ,AM_IMP ,6},{IN_EOR ,AM_INDX,6},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_EOR ,AM_ZPG ,3},{IN_LSR ,AM_ZPG ,5},{IN_RMB4,AM_ZPG ,5},{IN_PHA ,AM_IMP ,3},{IN_EOR ,AM_IMM ,2},{IN_LSR ,AM_ACC ,2},{IN_XXX ,AM_XXX ,0},{IN_JMP ,AM_ABS ,3},{IN_EOR ,AM_ABS ,4},{IN_LSR ,AM_ABS ,6},{IN_BBR4,AM_REL ,5},
	{IN_BVC ,AM_REL ,2},{IN_EOR ,AM_INDY,5},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_EOR ,AM_ZPGX,4},{IN_LSR ,AM_ZPGX,6},{IN_RMB5,AM_ZPG ,5},{IN_CLI ,AM_IMP ,2},{IN_EOR ,AM_ABSY,4},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_EOR ,AM_ABSX,4},{IN_LSR ,AM_ABSX,7},{IN_BBR5,AM_REL ,5},
	{IN_RTS ,AM_IMP ,6},{IN_ADC ,AM_INDX,6},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_ADC ,AM_ZPG ,3},{IN_ROR ,AM_ZPG ,5},{IN_RMB6,AM_ZPG ,5},{IN_PLA ,AM_IMP ,4},{IN_ADC ,AM_IMM ,2},{IN_ROR ,AM_ACC ,2},{IN_XXX ,AM_XXX ,0},{IN_JMP ,AM_IND ,5},{IN_ADC ,AM_ABS ,4},{IN_ROR ,AM_ABS ,6},{IN_BBR6,AM_REL ,5},
	{IN_BVS ,AM_REL ,2},{IN_ADC ,AM_INDY,5},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_ADC ,AM_ZPGX,4},{IN_ROR ,AM_ZPGX,6},{IN_RMB7,AM_ZPG ,5},{IN_SEI ,AM_IMP ,2},{IN_ADC ,AM_ABSY,4},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_ADC ,AM_ABSX,4},{IN_ROR ,AM_ABSX,7},{IN_BBR7,AM_REL ,5},

	{IN_XXX ,AM_XXX ,0},{IN_STA ,AM_INDX,6},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_STY ,AM_ZPG ,3},{IN_STA ,AM_ZPG ,3},{IN_STX ,AM_ZPG ,3},{IN_SMB0,AM_ZPG ,5},{IN_DEY ,AM_IMP ,2},{IN_XXX ,AM_XXX ,0},{IN_TXA ,AM_IMP ,2},{IN_XXX ,AM_XXX ,0},{IN_STY ,AM_ABS ,4},{IN_STA ,AM_ABS ,4},{IN_STX ,AM_ABS ,4},{IN_BBS0,AM_REL ,5},
	{IN_BCC ,AM_REL ,2},{IN_STA ,AM_INDY,6},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_STY ,AM_ZPGX,4},{IN_STA ,AM_ZPGX,4},{IN_STX ,AM_ZPGY,4},{IN_SMB1,AM_ZPG ,5},{IN_TYA ,AM_IMP ,2},{IN_STA ,AM_ABSY,5},{IN_TXS ,AM_IMP ,2},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_STA ,AM_ABSX,5},{IN_XXX ,AM_XXX ,0},{IN_BBS1,AM_REL ,5},
	{IN_LDY ,AM_IMM ,2},{IN_LDA ,AM_INDX,6},{IN_LDX ,AM_IMM ,2},{IN_XXX ,AM_XXX ,0},{IN_LDY ,AM_ZPG ,3},{IN_LDA ,AM_ZPG ,3},{IN_LDX ,AM_ZPG ,3},{IN_SMB2,AM_ZPG ,5},{IN_TAY ,AM_IMP ,2},{IN_LDA ,AM_IMM ,2},{IN_TAX ,AM_IMP ,2},{IN_XXX ,AM_XXX ,0},{IN_LDY ,AM_ABS ,4},{IN_LDA ,AM_ABS ,4},{IN_LDX ,AM_ABS ,4},{IN_BBS2,AM_REL ,5},
	{IN_BCS ,AM_REL ,2},{IN_LDA ,AM_INDY,5},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_LDY ,AM_ZPGX,4},{IN_LDA ,AM_ZPGX,4},{IN_LDX ,AM_ZPGY,4},{IN_SMB3,AM_ZPG ,5},{IN_CLV ,AM_IMP ,2},{IN_LDA ,AM_ABSY,4},{IN_TSX ,AM_IMP ,2},{IN_XXX ,AM_XXX ,0},{IN_LDY ,AM_ABSX,4},{IN_LDA ,AM_ABSX,4},{IN_LDX ,AM_ABSY,4},{IN_BBS3,AM_REL ,5},
	{IN_CPY ,AM_IMM ,2},{IN_CMP ,AM_INDX,6},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_CPY ,AM_ZPG ,3},{IN_CMP ,AM_ZPG ,3},{IN_DEC ,AM_ZPG ,5},{IN_SMB4,AM_ZPG ,5},{IN_INY ,AM_IMP ,2},{IN_CMP ,AM_IMM ,2},{IN_DEX ,AM_IMP ,2},{IN_XXX ,AM_XXX ,0},{IN_CPY ,AM_ABS ,4},{IN_CMP ,AM_ABS ,4},{IN_DEC ,AM_ABS ,6},{IN_BBS4,AM_REL ,5},
	{IN_BNE ,AM_REL ,2},{IN_CMP ,AM_INDY,5},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_CMP ,AM_ZPGX,4},{IN_DEC ,AM_ZPGX,6},{IN_SMB5,AM_ZPG ,5},{IN_CLD ,AM_IMP ,2},{IN_CMP ,AM_ABSY,4},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_CMP ,AM_ABSX,4},{IN_DEC ,AM_ABSX,7},{IN_BBS5,AM_REL ,5},
	{IN_CPX ,AM_IMM ,2},{IN_SBC ,AM_INDX,6},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_CPX ,AM_ZPG ,3},{IN_SBC ,AM_ZPG ,3},{IN_INC ,AM_ZPG ,5},{IN_SMB6,AM_ZPG ,5},{IN_INX ,AM_IMP ,2},{IN_SBC ,AM_IMM ,2},{IN_NOP ,AM_IMP ,2},{IN_XXX ,AM_XXX ,0},{IN_CPX ,AM_ABS ,4},{IN_SBC ,AM_ABS ,4},{IN_INC ,AM_ABS ,6},{IN_BBS6,AM_REL ,5},
	{IN_BEQ ,AM_REL ,2},{IN_SBC ,AM_INDY,5},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_SBC ,AM_ZPGX,4},{IN_INC ,AM_ZPGX,6},{IN_SMB7,AM_ZPG ,5},{IN_SED ,AM_IMP ,2},{IN_SBC ,AM_ABSY,4},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_SBC ,AM_ABSX,4},{IN_INC ,AM_ABSX,7},{IN_BBS7,AM_REL ,5},
};
#else
struct opcode opcodes[] = {
	{IN_BRK ,AM_IMP ,7},{IN_ORA ,AM_INDX,6},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_ORA ,AM_ZPG ,3},{IN_ASL ,AM_ZPG ,5},{IN_XXX ,AM_XXX ,0},{IN_PHP ,AM_IMP ,3},{IN_ORA ,AM_IMM ,2},{IN_ASL ,AM_ACC ,2},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_ORA ,AM_ABS ,4},{IN_ASL ,AM_ABS ,6},{IN_XXX ,AM_XXX ,0},
	{IN_BPL ,AM_REL ,2},{IN_ORA ,AM_INDY,5},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_ORA ,AM_ZPGX,4},{IN_ASL ,AM_ZPGX,6},{IN_XXX ,AM_XXX ,0},{IN_CLC ,AM_IMP ,2},{IN_ORA ,AM_ABSY,4},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_ORA ,AM_ABSX,4},{IN_ASL ,AM_ABSX,7},{IN_XXX ,AM_XXX ,0},
	{IN_JSR ,AM_ABS ,6},{IN_AND ,AM_INDX,6},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_BIT ,AM_ZPG ,3},{IN_AND ,AM_ZPG ,3},{IN_ROL ,AM_ZPG ,5},{IN_XXX ,AM_XXX ,0},{IN_PLP ,AM_IMP ,4},{IN_AND ,AM_IMM ,2},{IN_ROL ,AM_ACC ,2},{IN_XXX ,AM_XXX ,0},{IN_BIT ,AM_ABS ,4},{IN_AND ,AM_ABS ,4},{IN_ROL ,AM_ABS ,6},{IN_XXX ,AM_XXX ,0},
	{IN_BMI ,AM_REL ,2},{IN_AND ,AM_INDY,5},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_AND ,AM_ZPGX,4},{IN_ROL ,AM_ZPGX,6},{IN_XXX ,AM_XXX ,0},{IN_SEC ,AM_IMP ,2},{IN_AND ,AM_ABSY,4},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_AND ,AM_ABSX,4},{IN_ROL ,AM_ABSX,7},{IN_XXX ,AM_XXX ,0},
	{IN_RTI ,AM_IMP ,6},{IN_EOR ,AM_INDX,6},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_EOR ,AM_ZPG ,3},{IN_LSR ,AM_ZPG ,5},{IN_XXX ,AM_XXX ,0},{IN_PHA ,AM_IMP ,3},{IN_EOR ,AM_IMM ,2},{IN_LSR ,AM_ACC ,2},{IN_XXX ,AM_XXX ,0},{IN_JMP ,AM_ABS ,3},{IN_EOR ,AM_ABS ,4},{IN_LSR ,AM_ABS ,6},{IN_XXX ,AM_XXX ,0},
	{IN_BVC ,AM_REL ,2},{IN_EOR ,AM_INDY,5},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_EOR ,AM_ZPGX,4},{IN_LSR ,AM_ZPGX,6},{IN_XXX ,AM_XXX ,0},{IN_CLI ,AM_IMP ,2},{IN_EOR ,AM_ABSY,4},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_EOR ,AM_ABSX,4},{IN_LSR ,AM_ABSX,7},{IN_XXX ,AM_XXX ,0},
	{IN_RTS ,AM_IMP ,6},{IN_ADC ,AM_INDX,6},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_ADC ,AM_ZPG ,3},{IN_ROR ,AM_ZPG ,5},{IN_XXX ,AM_XXX ,0},{IN_PLA ,AM_IMP ,4},{IN_ADC ,AM_IMM ,2},{IN_ROR ,AM_ACC ,2},{IN_XXX ,AM_XXX ,0},{IN_JMP ,AM_IND ,5},{IN_ADC ,AM_ABS ,4},{IN_ROR ,AM_ABS ,6},{IN_XXX ,AM_XXX ,0},
	{IN_BVS ,AM_REL ,2},{IN_ADC ,AM_INDY,5},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_ADC ,AM_ZPGX,4},{IN_ROR ,AM_ZPGX,6},{IN_XXX ,AM_XXX ,0},{IN_SEI ,AM_IMP ,2},{IN_ADC ,AM_ABSY,4},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_ADC ,AM_ABSX,4},{IN_ROR ,AM_ABSX,7},{IN_XXX ,AM_XXX ,0},

	{IN_XXX ,AM_XXX ,0},{IN_STA ,AM_INDX,6},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_STY ,AM_ZPG ,3},{IN_STA ,AM_ZPG ,3},{IN_STX ,AM_ZPG ,3},{IN_XXX ,AM_XXX ,0},{IN_DEY ,AM_IMP ,2},{IN_XXX ,AM_XXX ,0},{IN_TXA ,AM_IMP ,2},{IN_XXX ,AM_XXX ,0},{IN_STY ,AM_ABS ,4},{IN_STA ,AM_ABS ,4},{IN_STX ,AM_ABS ,4},{IN_XXX ,AM_XXX ,0},
	{IN_BCC ,AM_REL ,2},{IN_STA ,AM_INDY,6},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_STY ,AM_ZPGX,4},{IN_STA ,AM_ZPGX,4},{IN_STX ,AM_ZPGY,4},{IN_XXX ,AM_XXX ,0},{IN_TYA ,AM_IMP ,2},{IN_STA ,AM_ABSY,5},{IN_TXS ,AM_IMP ,2},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_STA ,AM_ABSX,5},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},
	{IN_LDY ,AM_IMM ,2},{IN_LDA ,AM_INDX,6},{IN_LDX ,AM_IMM ,2},{IN_XXX ,AM_XXX ,0},{IN_LDY ,AM_ZPG ,3},{IN_LDA ,AM_ZPG ,3},{IN_LDX ,AM_ZPG ,3},{IN_XXX ,AM_XXX ,0},{IN_TAY ,AM_IMP ,2},{IN_LDA ,AM_IMM ,2},{IN_TAX ,AM_IMP ,2},{IN_XXX ,AM_XXX ,0},{IN_LDY ,AM_ABS ,4},{IN_LDA ,AM_ABS ,4},{IN_LDX ,AM_ABS ,4},{IN_XXX ,AM_XXX ,0},
	{IN_BCS ,AM_REL ,2},{IN_LDA ,AM_INDY,5},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_LDY ,AM_ZPGX,4},{IN_LDA ,AM_ZPGX,4},{IN_LDX ,AM_ZPGY,4},{IN_XXX ,AM_XXX ,0},{IN_CLV ,AM_IMP ,2},{IN_LDA ,AM_ABSY,4},{IN_TSX ,AM_IMP ,2},{IN_XXX ,AM_XXX ,0},{IN_LDY ,AM_ABSX,4},{IN_LDA ,AM_ABSX,4},{IN_LDX ,AM_ABSY,4},{IN_XXX ,AM_XXX ,0},
	{IN_CPY ,AM_IMM ,2},{IN_CMP ,AM_INDX,6},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_CPY ,AM_ZPG ,3},{IN_CMP ,AM_ZPG ,3},{IN_DEC ,AM_ZPG ,5},{IN_XXX ,AM_XXX ,0},{IN_INY ,AM_IMP ,2},{IN_CMP ,AM_IMM ,2},{IN_DEX ,AM_IMP ,2},{IN_XXX ,AM_XXX ,0},{IN_CPY ,AM_ABS ,4},{IN_CMP ,AM_ABS ,4},{IN_DEC ,AM_ABS ,6},{IN_XXX ,AM_XXX ,0},
	{IN_BNE ,AM_REL ,2},{IN_CMP ,AM_INDY,5},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_CMP ,AM_ZPGX,4},{IN_DEC ,AM_ZPGX,6},{IN_XXX ,AM_XXX ,0},{IN_CLD ,AM_IMP ,2},{IN_CMP ,AM_ABSY,4},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_CMP ,AM_ABSX,4},{IN_DEC ,AM_ABSX,7},{IN_XXX ,AM_XXX ,0},
	{IN_CPX ,AM_IMM ,2},{IN_SBC ,AM_INDX,6},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_CPX ,AM_ZPG ,3},{IN_SBC ,AM_ZPG ,3},{IN_INC ,AM_ZPG ,5},{IN_XXX ,AM_XXX ,0},{IN_INX ,AM_IMP ,2},{IN_SBC ,AM_IMM ,2},{IN_NOP ,AM_IMP ,2},{IN_XXX ,AM_XXX ,0},{IN_CPX ,AM_ABS ,4},{IN_SBC ,AM_ABS ,4},{IN_INC ,AM_ABS ,6},{IN_XXX ,AM_XXX ,0},
	{IN_BEQ ,AM_REL ,2},{IN_SBC ,AM_INDY,5},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_SBC ,AM_ZPGX,4},{IN_INC ,AM_ZPGX,6},{IN_XXX ,AM_XXX ,0},{IN_SED ,AM_IMP ,2},{IN_SBC ,AM_ABSY,4},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_XXX ,AM_XXX ,0},{IN_SBC ,AM_ABSX,4},{IN_INC ,AM_ABSX,7},{IN_XXX ,AM_XXX ,0},
};
#endif

#ifdef VERBOSE
#define AM_ENTRY(name) { #name, am_##name }
#else
#define AM_ENTRY(name) { am_##name }
#endif
struct addressMode addressModes[] = {
	[AM_ABS]  = AM_ENTRY(abs ),
#ifdef WDC
	[AM_ABSI] = AM_ENTRY(absi),
#endif
	[AM_ABSX] = AM_ENTRY(absx),
	[AM_ABSY] = AM_ENTRY(absy),
#ifdef VERBOSE
	[AM_ACC]  = {"acc", am_imp},
#else
	[AM_ACC]  = {am_imp},
#endif
	[AM_IMM]  = AM_ENTRY(imm ),
	[AM_IMP]  = AM_ENTRY(imp ),
	[AM_IND]  = AM_ENTRY(ind ),
	[AM_INDX] = AM_ENTRY(indx),
	[AM_INDY] = AM_ENTRY(indy),
	[AM_REL]  = AM_ENTRY(rel ),
#ifdef WDC
#ifdef VERBOSE
	[AM_STK]  = {"stk", am_imp},
#else
	[AM_STK]  = {am_imp},
#endif
#endif
	[AM_ZPG]  = AM_ENTRY(zpg ),
#ifdef WDC
	[AM_ZPGI] = AM_ENTRY(zpgi),
#endif
	[AM_ZPGX] = AM_ENTRY(zpgx),
	[AM_ZPGY] = AM_ENTRY(zpgy),

#ifndef WDC
	[AM_XXX]  = AM_ENTRY(xxx ),
#endif
};
#undef AM_ENTRY

#ifdef VERBOSE
#define IN_ENTRY(name) { #name, in_##name }
#else
#define IN_ENTRY(name) { in_##name }
#endif
struct instruction instructions[] = {
	[IN_ADC]  = IN_ENTRY(adc ),
	[IN_AND]  = IN_ENTRY(and ),
	[IN_ASL]  = IN_ENTRY(asl ),
#ifdef ROCKWEL
	[IN_BBR0] = IN_ENTRY(bbr0),
	[IN_BBR1] = IN_ENTRY(bbr1),
	[IN_BBR2] = IN_ENTRY(bbr2),
	[IN_BBR3] = IN_ENTRY(bbr3),
	[IN_BBR4] = IN_ENTRY(bbr4),
	[IN_BBR5] = IN_ENTRY(bbr5),
	[IN_BBR6] = IN_ENTRY(bbr6),
	[IN_BBR7] = IN_ENTRY(bbr7),
#endif
#ifdef ROCKWEL
	[IN_BBS0] = IN_ENTRY(bbs0),
	[IN_BBS1] = IN_ENTRY(bbs1),
	[IN_BBS2] = IN_ENTRY(bbs2),
	[IN_BBS3] = IN_ENTRY(bbs3),
	[IN_BBS4] = IN_ENTRY(bbs4),
	[IN_BBS5] = IN_ENTRY(bbs5),
	[IN_BBS6] = IN_ENTRY(bbs6),
	[IN_BBS7] = IN_ENTRY(bbs7),
#endif
	[IN_BCC]  = IN_ENTRY(bcc ),
	[IN_BCS]  = IN_ENTRY(bcs ),
	[IN_BEQ]  = IN_ENTRY(beq ),
	[IN_BIT]  = IN_ENTRY(bit ),
	[IN_BMI]  = IN_ENTRY(bmi ),
	[IN_BNE]  = IN_ENTRY(bne ),
	[IN_BPL]  = IN_ENTRY(bpl ),
#ifdef WDC
	[IN_BRA]  = IN_ENTRY(bra ),
#endif
	[IN_BRK]  = IN_ENTRY(brk ),
	[IN_BVC]  = IN_ENTRY(bvc ),
	[IN_BVS]  = IN_ENTRY(bvs ),
	[IN_CLC]  = IN_ENTRY(clc ),
	[IN_CLD]  = IN_ENTRY(cld ),
	[IN_CLI]  = IN_ENTRY(cli ),
	[IN_CLV]  = IN_ENTRY(clv ),
	[IN_CMP]  = IN_ENTRY(cmp ),
	[IN_CPX]  = IN_ENTRY(cpx ),
	[IN_CPY]  = IN_ENTRY(cpy ),
	[IN_DEC]  = IN_ENTRY(dec ),
	[IN_DEX]  = IN_ENTRY(dex ),
	[IN_DEY]  = IN_ENTRY(dey ),
	[IN_EOR]  = IN_ENTRY(eor ),
	[IN_INC]  = IN_ENTRY(inc ),
	[IN_INX]  = IN_ENTRY(inx ),
	[IN_INY]  = IN_ENTRY(iny ),
	[IN_JMP]  = IN_ENTRY(jmp ),
	[IN_JSR]  = IN_ENTRY(jsr ),
	[IN_LDA]  = IN_ENTRY(lda ),
	[IN_LDX]  = IN_ENTRY(ldx ),
	[IN_LDY]  = IN_ENTRY(ldy ),
	[IN_LSR]  = IN_ENTRY(lsr ),
	[IN_NOP]  = IN_ENTRY(nop ),
	[IN_ORA]  = IN_ENTRY(ora ),
	[IN_PHA]  = IN_ENTRY(pha ),
	[IN_PHP]  = IN_ENTRY(php ),
#ifdef WDC
	[IN_PHX]  = IN_ENTRY(phx ),
#endif
#ifdef WDC
	[IN_PHY]  = IN_ENTRY(phy ),
#endif
	[IN_PLA]  = IN_ENTRY(pla ),
	[IN_PLP]  = IN_ENTRY(plp ),
#ifdef WDC
	[IN_PLX]  = IN_ENTRY(plx ),
#endif
#ifdef WDC
	[IN_PLY]  = IN_ENTRY(ply ),
#endif
#ifdef ROCKWEL
	[IN_RMB0] = IN_ENTRY(rmb0),
	[IN_RMB1] = IN_ENTRY(rmb1),
	[IN_RMB2] = IN_ENTRY(rmb2),
	[IN_RMB3] = IN_ENTRY(rmb3),
	[IN_RMB4] = IN_ENTRY(rmb4),
	[IN_RMB5] = IN_ENTRY(rmb5),
	[IN_RMB6] = IN_ENTRY(rmb6),
	[IN_RMB7] = IN_ENTRY(rmb7),
#endif
	[IN_ROL]  = IN_ENTRY(rol ),
	[IN_ROR]  = IN_ENTRY(ror ),
	[IN_RTI]  = IN_ENTRY(rti ),
	[IN_RTS]  = IN_ENTRY(rts ),
	[IN_SBC]  = IN_ENTRY(sbc ),
	[IN_SEC]  = IN_ENTRY(sec ),
	[IN_SED]  = IN_ENTRY(sed ),
	[IN_SEI]  = IN_ENTRY(sei ),
#ifdef ROCKWEL
	[IN_SMB0] = IN_ENTRY(smb0),
	[IN_SMB1] = IN_ENTRY(smb1),
	[IN_SMB2] = IN_ENTRY(smb2),
	[IN_SMB3] = IN_ENTRY(smb3),
	[IN_SMB4] = IN_ENTRY(smb4),
	[IN_SMB5] = IN_ENTRY(smb5),
	[IN_SMB6] = IN_ENTRY(smb6),
	[IN_SMB7] = IN_ENTRY(smb7),
#endif
	[IN_STA]  = IN_ENTRY(sta ),
#ifdef WDC
	[IN_STP]  = IN_ENTRY(stp ),
#endif
	[IN_STX]  = IN_ENTRY(stx ),
	[IN_STY]  = IN_ENTRY(sty ),
#ifdef WDC
	[IN_STZ]  = IN_ENTRY(stz ),
#endif
	[IN_TAX]  = IN_ENTRY(tax ),
	[IN_TAY]  = IN_ENTRY(tay ),
#ifdef WDC
	[IN_TRB]  = IN_ENTRY(trb ),
#endif
#ifdef WDC
	[IN_TSB]  = IN_ENTRY(tsb ),
#endif
	[IN_TSX]  = IN_ENTRY(tsx ),
	[IN_TXA]  = IN_ENTRY(txa ),
	[IN_TXS]  = IN_ENTRY(txs ),
	[IN_TYA]  = IN_ENTRY(tya ),
#ifdef WDC
	[IN_WAI]  = IN_ENTRY(wai ),
#endif

#ifndef WDC
	[IN_XXX]  = IN_ENTRY(xxx ),
#endif
};
#undef IN_ENTRY

#pragma endregion data
