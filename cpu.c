#include "cpu.h"

#include "bus.h"

#ifdef WDC
// Western Design Center included the bit branch/bit set instructions from the rockwel chips
#define ROCKWEL
#endif

#ifdef ROCKWEL
// account for bit number in some newer instructions
#define INSTRUCTION_NAME_LENGTH 5
#else
#define INSTRUCTION_NAME_LENGTH 4
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

struct instructionSetEntry {
	const uint8_t instruction;
	const uint8_t addressMode;
	const uint8_t cycleCount;
} instructionSet[256];

struct addressModeEntry {
	const char name[5];
	bool (*const addressMode)();
} addressModes[ADDRESS_MODE_COUNT];

struct instructionEntry {
	const char name[INSTRUCTION_NAME_LENGTH];
	bool (*const instruction)();
} instructions[INSTRUCTION_COUNT];

size_t cycles = 0;
uint8_t currentOpcode = 0;

void push(uint8_t data) {
	bus_write(0x0100 | registers.SP--, data);
}

uint8_t pull() {
	return bus_read(0x0100 | registers.SP++);
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
	push(registers.flags);

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

	currentOpcode = bus_read(registers.PC++);

	const struct instructionSetEntry instruction = instructionSet[currentOpcode];

	// set cycles from instruction
	cycles = instruction.cycleCount;

	addressModes[instruction.addressMode].addressMode();
	instructions[instruction.instruction].instruction();
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

#pragma region addressingModes
// the following functions all implement logic for the addressing mode
// if they return true an additional clockcycle might need to be taken depending on the instruction


// absolute addressing mode
// an absolute memory location is provided
// the value at this memory location is used as operand
bool am_abs() {
	return false;
}

#ifdef WDC
// absolute indexed indirect addressing mode
// an absolute memory location is provided
// this memory location is incremented by X, this memory location provides 2 bytes to actually use, in the format $LLHH
// this mode is generally only used for JMP
bool am_absi() {
	return false;
}
#endif

// absolute addressing mode offset by X
// an absolute memory location is provided
// this memory location is incremented by X, and the value at that memory location is used as operand
// this can be used to loop through a set of data, aka an array
bool am_absx() {
	return false;
}

// absolute addressing mode offset by Y
// an absolute memory location is provided
// this memory location is incremented by Y, and the value at that memory location is used as operand
// this can be used to loop through a set of data, aka an array
bool am_absy() {
	return false;
}

// immediate addressing mode
// operand is provided directly after the instruction
bool am_imm() {
	return false;
}

// implied addressing mode
// operand is implied by the instruction
//   this also includes accumulator addressing mode, as accumulator is the implied operand
//   this also includes stack addressing mode documented in the WDC data sheets, as stack pointer is the implied operand
bool am_imp() {
	return false;
}

// indirect addressing mode
// an absolute memory location is provided
// this memory location provides 2 byte to actually use, in the format $LLHH
// this mode is generally only used for JMP
bool am_ind() {
	return false;
}

// pre-indexed indirect addressing mode
// a byte is provided, which describes the offset in the zero-page
// from this zero-page address two bytes are read ($LLHH), which gets incremented by X
// this increased memory address points to the memory address ($LLHH) where the actual data is stored
bool am_indx() {
	return false;
}

// post-indexed indirect addressing mode
// a byte is provided, which describes the offset in the zero-page
// from this zero-page address two bytes are read ($LLHH)
// this memory address points to the memory address ($LLHH), which gets incremented with Y
// this address is where the actual data is stored
bool am_indy() {
	return false;
}

// relative addressing mode
// operand provided is a signed byte
// this byte is added to PC to get the address to branch to
// this mode is only allowed for the branch instructions
bool am_rel() {
	return false;
}

// zero-page addressing mode
// a byte is provided directly after the instruction which contains an offset in the zero-page
// the absolute address would be $00XX
// this makes the operation faster, and shorter
// the instruction takes only 2 bytes, instead of 3 for a full address
bool am_zpg() {
	return false;
}

#ifdef WDC
// zero-page indirect addressing mode
// a byte is provided directly after the instruction which contains an offset in the zero-page
// this memory location provides 2 byte to actually use, in the format $LLHH
bool am_zpgi() {
	return false;
}
#endif

// zero-page addressing mode offset by X
// a byte is provided directly after the instruction which contains an offset in the zero-page
// this memory location is incremented by X, and the value at that memory location is used as operand
// this can be used to loop through a set of data, aka an array
// this makes the operation faster, and shorter
// the instruction takes only 2 bytes, instead of 3 for a full address
// the result of the addition wraps around, so the byte read will always be inside the zero-page
bool am_zpgx() {
	return false;
}

// zero-page addressing mode offset by Y
// a byte is provided directly after the instruction which contains an offset in the zero-page
// this memory location is incremented by Y, and the value at that memory location is used as operand
// this can be used to loop through a set of data, aka an array
// this makes the operation faster, and shorter
// the instruction takes only 2 bytes, instead of 3 for a full address
// the result of the addition wraps around, so the byte read will always be inside the zero-page
// this addressing mode is only used if the register used is X (LDX, STX), so X cannot be used
bool am_zpgy() {
	return false;
}

#ifndef WDC
// addressing mode for illegal opcodes
// shouldn't be used in normal programs
//   the 6502 has a couple of opcodes that gave somewhat reliable results
//   the WDC version of the 6502 has all illegal opcodes as implemented as nops
bool am_xxx() {
	return false;
}
#endif

#pragma endregion addressingModes

#pragma region instructions
// the following functions all implement logic for the instructions
// if they return true an additional clockcycle might need to be taken depending on the addressing mode

#ifdef ROCKWEL
// some macro magic to get an easier time implementing the bit instructions
// these instrutions originated in the Rockwel chips, and were later adapted by WDC
#define BITS_EXPANSION(funcName) \
bool in_##funcName(uint8_t bit); \
BIT_EXPANSION(funcName, 0) BIT_EXPANSION(funcName, 1) \
BIT_EXPANSION(funcName, 2) BIT_EXPANSION(funcName, 3) BIT_EXPANSION(funcName, 4) \
BIT_EXPANSION(funcName, 5) BIT_EXPANSION(funcName, 6) BIT_EXPANSION(funcName, 7)
#define BIT_EXPANSION(funcName, bit) \
bool in_##funcName##bit() { return in_##funcName(bit); }
#endif

// ADd with Carry
// adds operand to accumulator with carry flag
bool in_adc() {
	return false;
}

// bitwise AND
// performs a bitwise and with operand and accumulator
bool in_and() {
	return false;
}

// Arithmatic Shift Left
// shifts 0 into bit 0
// shifts bit 7 into carry flag
bool in_asl() {
	return false;
}

#ifdef ROCKWEL
// Branch on Bit Reset
// tests bit of accumulator, and branches if it is 0
// a branch taken takes an extra clock cycle
bool in_bbr(uint8_t bit) {
	return false;
}

BITS_EXPANSION(bbr)
#endif

#ifdef ROCKWEL
// Branch on Bit Set
// tests bit of accumulator, and branches if it is 1
// a branch taken takes an extra clock cycle
bool in_bbs(uint8_t bit) {
	return false;
}

BITS_EXPANSION(bbs)
#endif

// Branch Carry Clear
// branches when carry flag is unset
// a branch taken takes an extra clock cycle
bool in_bcc() {
	return false;
}

// Branch Carry Set
// branches when carry flag is set
// a branch taken takes an extra clock cycle
bool in_bcs() {
	return false;
}

// Branch on EQual
// branches when zero flag is set (two values are equal if A - B == 0)
// a branch taken takes an extra clock cycle
bool in_beq() {
	return false;
}

// test BITs
// probably the most confusing instruction of the 6502
// zero flag is set according to the operation accumulator AND operand
// bits 6 and 7 of operand are set as negative and overflow flags respectively
// this instruction only alters flags register
bool in_bit() {
	return false;
}

// Branch on MInus
// branches when negative flag is set
// a branch taken takes an extra clock cycle
bool in_bmi() {
	return false;
}

// Branch on Not Equals
// branches when zero flag is unset (two values are not equal if A - B != 0)
// a branch taken takes an extra clock cycle
bool in_bne() {
	return false;
}

// Branch on PLus
// branches when negative flag is unset
// a branch taken takes an extra clock cycle
bool in_bpl() {
	return false;
}

#ifdef WDC
// Branch Always
// a branch taken takes an extra clock cycle
//   this is always the case
bool in_bra() {
	return true;
}
#endif

// BReaK
// forces an interupt to occur
// this starts the IRQ sequence with break flag set
// the return address is PC + 2, making the byte following this instruction being skipped
// this byte can be used as a break mark
bool in_brk() {
	return false;
}

// Branch on oVerflow Clear
// branches when overflow flag is unset
// a branch taken takes an extra clock cycle
bool in_bvc() {
	return false;
}

// Branch on oVerflow Set
// branches when overflow flag is set
// a branch taken takes an extra clock cycle
bool in_bvs() {
	return false;
}

// CLear Carry flag
bool in_clc() {
	return false;
}

// CLear Decimal flag
// makes the 6502 perform normal binary math
bool in_cld() {
	return false;
}

// CLear Interupt flag
// this instruction enables interupts from the IRQ pin/function, as this pin is active low
bool in_cli() {
	return false;
}

// CLear oVerflow
bool in_clv() {
	return false;
}

// CoMPare with accumulator
// subtract operand with accumulator, and set flags register accordingly
// then ignore the result from the subtraction
// this instruction only alters flags register
bool in_cmp() {
	return false;
}

// CoMpare with X register
// subtract operand with accumulator, and set flags register accordingly
// then ignore the result from the subtraction
// this instruction only alters flags register
bool in_cpx() {
	return false;
}

// CoMpare with Y register
// subtract operand with accumulator, and set flags register accordingly
// then ignore the result from the subtraction
// this instruction only alters flags register
bool in_cpy() {
	return false;
}

// DECrement operand
bool in_dec() {
	return false;
}

// DEcrement X register
bool in_dex() {
	return false;
}

// DEcrement Y register
bool in_dey() {
	return false;
}

// bitwise eXclusive OR
// performs a bitwise exclusive or with operand and accumulator
bool in_eor() {
	return false;
}

// INCrement operand
bool in_inc() {
	return false;
}

// INcrement X register
bool in_inx() {
	return false;
}

// INcrement Y register
bool in_iny() {
	return false;
}

// JuMP
// loads PC with operand
bool in_jmp() {
	return false;
}

// Jump to SubRoutine
// saves current PC in stack
// loads PC with operand
bool in_jsr() {
	return false;
}

// LoaD Accumulator with operand
bool in_lda() {
	return false;
}

// LoaD X register with operand
bool in_ldx() {
	return false;
}

// LoaD Y register with operand
bool in_ldy() {
	return false;
}

// Logical Shift Right
// shifts 0 into bit 7
// shifts bit 0 into carry flag
bool in_lsr() {
	return false;
}

// No OPeration
// this instruction does nothing
//   the WDC version of the 6502 has all illegal opcodes as implemented as nops
//   these differ slightly in operand size and/or cycle counts
bool in_nop() {
	return false;
}

// bitwise OR with Accumulator
// performs a bitwise or with operand and accumulator
bool in_ora() {
	return false;
}

// PusH Accumulator on stack
bool in_pha() {
	return false;
}

// PusH Processor status
// pushes flags register on stack
// this instruction sets break flag and bit 5 (unused)
bool in_php() {
	return false;
}

#ifdef WDC
// PusH X register on stack
bool in_phx() {
	return false;
}
#endif

#ifdef WDC
// PusH Y register on stack
bool in_phy() {
	return false;
}
#endif

// PuLl Accumulator off stack
bool in_pla() {
	return false;
}

// PuLl Processor status
// pulls flags register off stack
// this instruction ignores break flag and bit 5 (unused)
bool in_plp() {
	return false;
}

#ifdef WDC
// PuLl X register off stack
bool in_plx() {
	return false;
}
#endif

#ifdef WDC
// PuLl Y register off stack
bool in_ply() {
	return false;
}
#endif

#ifdef ROCKWEL
// Reset Memory Bit
// sets bit at operand to 0
bool in_rmb(uint8_t bit) {
	return false;
}

BITS_EXPANSION(rmb)
#endif

// ROtate Left
// shifts carry flag into bit 0
// shifts bit 7 into carry flag
bool in_rol() {
	return false;
}

// ROtate Right
// shifts carry flag into bit 7
// shifts bit 0 into carry flag
//   early versions of the 6502 have a bug in this instruction
//   the instruction would instead perform as an ASL, without shifting bit 7 into carry flag
//   this bug makes it so that carry flag would be unused during the instruction
bool in_ror() {
	return false;
}

// ReTurn from Interupt
// pulls flags register from stack, ignoring break flag and bit 5 (ignored)
// then pulls PC from stack
bool in_rti() {
	return false;
}

// ReTurn from Subroutine
// pulls PC from stack
bool in_rts() {
	return false;
}

// SuBtract with Carry
// subtract operand from accumulator with carry flag as a borrow
// carry flag should be set before being called
// if carry flag is cleared after the call, a borrow was needed
bool in_sbc() {
	return false;
}

// SEt Carry flag
bool in_sec() {
	return false;
}

// SEt Decimal flag
// makes the 6502 perform binary coded decimal math
bool in_sed() {
	return false;
}

// SEt Interupt flag
// this instruction disables interupts from the IRQ pin/function, as this pin is active low
bool in_sei() {
	return false;
}

#ifdef ROCKWEL
// Set Memory Bit
// sets bit at operand to 1
bool in_smb(uint8_t bit) {
	return false;
}

BITS_EXPANSION(smb)
#endif

// STore Accumulator at operand
bool in_sta() {
	return false;
}

#ifdef WDC
// SToP
// stops the clock from affecting the 65c02
// the 65c02 is faster to respond to the reset pin/function
bool in_stp() {
	return false;
}
#endif

// STore X register at operand
bool in_stx() {
	return false;
}

// STore Y register at operand
bool in_sty() {
	return false;
}

#ifdef WDC
// STore Zero at operand
bool in_stz() {
	return false;
}
#endif

// Transfer Accumulator to X register
bool in_tax() {
	return false;
}

// Transfer Accumulator to Y register
bool in_tay() {
	return false;
}

#ifdef WDC
// Test and Reset Bit
// clears bits set in accumulator at operand
// sets zero flag if any bits were changed, otherwise it gets cleared
bool in_trb() {
	return false;
}
#endif

#ifdef WDC
// Test and Set Bit
// sets bits set in accumulator at operand
// sets zero flag if any bits were changed, otherwise it gets cleared
bool in_tsb() {
	return false;
}
#endif

// Transfer Stack pointer to X register
bool in_tsx() {
	return false;
}

// Transfer X register to Accumulator
bool in_txa() {
	return false;
}

// Transfer X register to Stack pointer
bool in_txs() {
	return false;
}

// Transfer Y register to Accumulator
bool in_tya() {
	return false;
}

#ifdef WDC
// WAit for Interupt
// stops the clock from affecting the 65c02
// the 65c02 is faster to respond to the IRQ/NMI pins/functions
bool in_wai() {
	return false;
}
#endif

#ifndef WDC
// instruction for illegal opcodes
// shouldn't be used in normal programs
//   the 6502 has a couple of opcodes that gave somewhat reliable results
//   the WDC version of the 6502 has all illegal opcodes as implemented as nops
bool in_xxx() {
	return false;
}
#endif

#ifdef ROCKWEL
#undef BITS_EXPANSION
#undef BIT_EXPANSION
#endif

#pragma endregion instructions

#pragma region data

#if defined(WDC)
#elif defined(ROCKWEL)
#else
struct instructionSetEntry instructionSet[] = {
	{IN_BRK,AM_IMP ,7},{IN_ORA,AM_INDX,6},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_ORA,AM_ZPG ,3},{IN_ASL,AM_ZPG ,5},{IN_XXX,AM_XXX ,0},{IN_PHP,AM_IMP ,3},{IN_ORA,AM_IMM ,2},{IN_ASL,AM_ACC ,2},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_ORA,AM_ABS ,4},{IN_ASL,AM_ABS ,6},{IN_XXX,AM_XXX ,0},
	{IN_BPL,AM_REL ,2},{IN_ORA,AM_INDY,5},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_ORA,AM_ZPGX,4},{IN_ASL,AM_ZPGX,6},{IN_XXX,AM_XXX ,0},{IN_CLC,AM_IMP ,2},{IN_ORA,AM_ABSY,4},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_ORA,AM_ABSX,4},{IN_ASL,AM_ABSX,7},{IN_XXX,AM_XXX ,0},
	{IN_JSR,AM_ABS ,6},{IN_AND,AM_INDX,6},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_BIT,AM_ZPG ,3},{IN_AND,AM_ZPG ,3},{IN_ROL,AM_ZPG ,5},{IN_XXX,AM_XXX ,0},{IN_PLP,AM_IMP ,4},{IN_AND,AM_IMM ,2},{IN_ROL,AM_ACC ,2},{IN_XXX,AM_XXX ,0},{IN_BIT,AM_ABS ,4},{IN_AND,AM_ABS ,4},{IN_ROL,AM_ABS ,6},{IN_XXX,AM_XXX ,0},
	{IN_BMI,AM_REL ,2},{IN_AND,AM_INDY,5},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_AND,AM_ZPGX,4},{IN_ROL,AM_ZPGX,6},{IN_XXX,AM_XXX ,0},{IN_SEC,AM_IMP ,2},{IN_AND,AM_ABSY,4},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_AND,AM_ABSX,4},{IN_ROL,AM_ABSX,7},{IN_XXX,AM_XXX ,0},
	{IN_RTI,AM_IMP ,6},{IN_EOR,AM_INDX,6},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_EOR,AM_ZPG ,3},{IN_LSR,AM_ZPG ,5},{IN_XXX,AM_XXX ,0},{IN_PHA,AM_IMP ,3},{IN_EOR,AM_IMM ,2},{IN_LSR,AM_ACC ,2},{IN_XXX,AM_XXX ,0},{IN_JMP,AM_ABS ,3},{IN_EOR,AM_ABS ,4},{IN_LSR,AM_ABS ,6},{IN_XXX,AM_XXX ,0},
	{IN_BVC,AM_REL ,2},{IN_EOR,AM_INDY,5},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_EOR,AM_ZPGX,4},{IN_LSR,AM_ZPGX,6},{IN_XXX,AM_XXX ,0},{IN_CLI,AM_IMP ,2},{IN_EOR,AM_ABSY,4},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_EOR,AM_ABSX,4},{IN_LSR,AM_ABSX,7},{IN_XXX,AM_XXX ,0},
	{IN_RTS,AM_IMP ,6},{IN_ADC,AM_INDX,6},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_ADC,AM_ZPG ,3},{IN_ROR,AM_ZPG ,5},{IN_XXX,AM_XXX ,0},{IN_PLA,AM_IMP ,4},{IN_ADC,AM_IMM ,2},{IN_ROR,AM_ACC ,2},{IN_XXX,AM_XXX ,0},{IN_JMP,AM_IND ,5},{IN_ADC,AM_ABS ,4},{IN_ROR,AM_ABS ,6},{IN_XXX,AM_XXX ,0},
	{IN_BVS,AM_REL ,2},{IN_ADC,AM_INDY,5},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_ADC,AM_ZPGX,4},{IN_ROR,AM_ZPGX,6},{IN_XXX,AM_XXX ,0},{IN_SEI,AM_IMP ,2},{IN_ADC,AM_ABSY,4},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_ADC,AM_ABSX,4},{IN_ROR,AM_ABSX,7},{IN_XXX,AM_XXX ,0},

	{IN_XXX,AM_XXX ,0},{IN_STA,AM_INDX,6},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_STY,AM_ZPG ,3},{IN_STA,AM_ZPG ,3},{IN_STX,AM_ZPG ,3},{IN_XXX,AM_XXX ,0},{IN_DEY,AM_IMP ,2},{IN_XXX,AM_XXX ,0},{IN_TXA,AM_IMP ,2},{IN_XXX,AM_XXX ,0},{IN_STY,AM_ABS ,4},{IN_STA,AM_ABS ,4},{IN_STX,AM_ABS ,4},{IN_XXX,AM_XXX ,0},
	{IN_BCC,AM_REL ,2},{IN_STA,AM_INDY,6},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_STY,AM_ZPGX,4},{IN_STA,AM_ZPGX,4},{IN_STX,AM_ZPGY,4},{IN_XXX,AM_XXX ,0},{IN_TYA,AM_IMP ,2},{IN_STA,AM_ABSY,5},{IN_TXS,AM_IMP ,2},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_STA,AM_ABSX,5},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},
	{IN_LDY,AM_IMM ,2},{IN_LDA,AM_INDX,6},{IN_LDX,AM_IMM ,2},{IN_XXX,AM_XXX ,0},{IN_LDY,AM_ZPG ,3},{IN_LDA,AM_ZPG ,3},{IN_LDX,AM_ZPG ,3},{IN_XXX,AM_XXX ,0},{IN_TAY,AM_IMP ,2},{IN_LDA,AM_IMM ,2},{IN_TAX,AM_IMP ,2},{IN_XXX,AM_XXX ,0},{IN_LDY,AM_ABS ,4},{IN_LDA,AM_ABS ,4},{IN_LDX,AM_ABS ,4},{IN_XXX,AM_XXX ,0},
	{IN_BCS,AM_REL ,2},{IN_LDA,AM_INDY,5},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_LDY,AM_ZPGX,4},{IN_LDA,AM_ZPGX,4},{IN_LDX,AM_ZPGY,4},{IN_XXX,AM_XXX ,0},{IN_CLV,AM_IMP ,2},{IN_LDA,AM_ABSY,4},{IN_TSX,AM_IMP ,2},{IN_XXX,AM_XXX ,0},{IN_LDY,AM_ABSX,4},{IN_LDA,AM_ABSX,4},{IN_LDX,AM_ABSY,4},{IN_XXX,AM_XXX ,0},
	{IN_CPY,AM_IMM ,2},{IN_CMP,AM_INDX,6},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_CPY,AM_ZPG ,3},{IN_CMP,AM_ZPG ,3},{IN_DEC,AM_ZPG ,5},{IN_XXX,AM_XXX ,0},{IN_INY,AM_IMP ,2},{IN_CMP,AM_IMM ,2},{IN_DEX,AM_IMP ,2},{IN_XXX,AM_XXX ,0},{IN_CPY,AM_ABS ,4},{IN_CMP,AM_ABS ,4},{IN_DEC,AM_ABS ,6},{IN_XXX,AM_XXX ,0},
	{IN_BNE,AM_REL ,2},{IN_CMP,AM_INDY,5},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_CMP,AM_ZPGX,4},{IN_DEC,AM_ZPGX,6},{IN_XXX,AM_XXX ,0},{IN_CLD,AM_IMP ,2},{IN_CMP,AM_ABSY,4},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_CMP,AM_ABSX,4},{IN_DEC,AM_ABSX,7},{IN_XXX,AM_XXX ,0},
	{IN_CPX,AM_IMM ,2},{IN_SBC,AM_INDX,6},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_CPX,AM_ZPG ,3},{IN_SBC,AM_ZPG ,3},{IN_INC,AM_ZPG ,5},{IN_XXX,AM_XXX ,0},{IN_INX,AM_IMP ,2},{IN_SBC,AM_IMM ,2},{IN_NOP,AM_IMP ,2},{IN_XXX,AM_XXX ,0},{IN_CPX,AM_ABS ,4},{IN_SBC,AM_ABS ,4},{IN_INC,AM_ABS ,6},{IN_XXX,AM_XXX ,0},
	{IN_BEQ,AM_REL ,2},{IN_SBC,AM_INDY,5},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_SBC,AM_ZPGX,4},{IN_INC,AM_ZPGX,6},{IN_XXX,AM_XXX ,0},{IN_SED,AM_IMP ,2},{IN_SBC,AM_ABSY,4},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_XXX,AM_XXX ,0},{IN_SBC,AM_ABSX,4},{IN_INC,AM_ABSX,7},{IN_XXX,AM_XXX ,0},
};
#endif

struct addressModeEntry addressModes[] = {
	[AM_ABS]  = {"abs",  am_abs },
#ifdef WDC
	[AM_ABSI] = {"absi", am_absi},
#endif
	[AM_ABSX] = {"absx", am_absx},
	[AM_ABSY] = {"absy", am_absy},
	[AM_ACC]  = {"acc",  am_imp },
	[AM_IMM]  = {"imm",  am_imm },
	[AM_IMP]  = {"imp",  am_imp },
	[AM_IND]  = {"ind",  am_ind },
	[AM_INDX] = {"indx", am_indx},
	[AM_INDY] = {"indy", am_indy},
	[AM_REL]  = {"rel",  am_rel },
#ifdef WDC
	[AM_STK]  = {"stk",  am_imp },
#endif
	[AM_ZPG]  = {"zpg",  am_zpg },
#ifdef WDC
	[AM_ZPGI] = {"zpgi", am_zpgi},
#endif
	[AM_ZPGX] = {"zpgx", am_zpgx},
	[AM_ZPGY] = {"zpgy", am_zpgy},

#ifndef WDC
	[AM_XXX]  = {"",     am_xxx },
#endif
};

struct instructionEntry instructions[] = {
	[IN_ADC]  = {"adc",  in_adc },
	[IN_AND]  = {"and",  in_and },
	[IN_ASL]  = {"asl",  in_asl },
#ifdef ROCKWEL
	[IN_BBR0] = {"bbr0", in_bbr0},
	[IN_BBR1] = {"bbr1", in_bbr1},
	[IN_BBR2] = {"bbr2", in_bbr2},
	[IN_BBR3] = {"bbr3", in_bbr3},
	[IN_BBR4] = {"bbr4", in_bbr4},
	[IN_BBR5] = {"bbr5", in_bbr5},
	[IN_BBR6] = {"bbr6", in_bbr6},
	[IN_BBR7] = {"bbr7", in_bbr7},
#endif
#ifdef ROCKWEL
	[IN_BBS0] = {"bbs0", in_bbs0},
	[IN_BBS1] = {"bbs1", in_bbs1},
	[IN_BBS2] = {"bbs2", in_bbs2},
	[IN_BBS3] = {"bbs3", in_bbs3},
	[IN_BBS4] = {"bbs4", in_bbs4},
	[IN_BBS5] = {"bbs5", in_bbs5},
	[IN_BBS6] = {"bbs6", in_bbs6},
	[IN_BBS7] = {"bbs7", in_bbs7},
#endif
	[IN_BCC]  = {"bcc",  in_bcc },
	[IN_BCS]  = {"bcs",  in_bcs },
	[IN_BEQ]  = {"beq",  in_beq },
	[IN_BIT]  = {"bit",  in_bit },
	[IN_BMI]  = {"bmi",  in_bmi },
	[IN_BNE]  = {"bne",  in_bne },
	[IN_BPL]  = {"bpl",  in_bpl },
#ifdef WDC
	[IN_BRA]  = {"bra",  in_bra },
#endif
	[IN_BRK]  = {"brk",  in_brk },
	[IN_BVC]  = {"bvc",  in_bvc },
	[IN_BVS]  = {"bvs",  in_bvs },
	[IN_CLC]  = {"clc",  in_clc },
	[IN_CLD]  = {"cld",  in_cld },
	[IN_CLI]  = {"cli",  in_cli },
	[IN_CLV]  = {"clv",  in_clv },
	[IN_CMP]  = {"cmp",  in_cmp },
	[IN_CPX]  = {"cpx",  in_cpx },
	[IN_CPY]  = {"cpy",  in_cpy },
	[IN_DEC]  = {"dec",  in_dec },
	[IN_DEX]  = {"dex",  in_dex },
	[IN_DEY]  = {"dey",  in_dey },
	[IN_EOR]  = {"eor",  in_eor },
	[IN_INC]  = {"inc",  in_inc },
	[IN_INX]  = {"inx",  in_inx },
	[IN_INY]  = {"iny",  in_iny },
	[IN_JMP]  = {"jmp",  in_jmp },
	[IN_JSR]  = {"jsr",  in_jsr },
	[IN_LDA]  = {"lda",  in_lda },
	[IN_LDX]  = {"ldx",  in_ldx },
	[IN_LDY]  = {"ldy",  in_ldy },
	[IN_LSR]  = {"lsr",  in_lsr },
	[IN_NOP]  = {"nop",  in_nop },
	[IN_ORA]  = {"ora",  in_ora },
	[IN_PHA]  = {"pha",  in_pha },
	[IN_PHP]  = {"php",  in_php },
#ifdef WDC
	[IN_PHX]  = {"phx",  in_phx },
#endif
#ifdef WDC
	[IN_PHY]  = {"phy",  in_phy },
#endif
	[IN_PLA]  = {"pla",  in_pla },
	[IN_PLP]  = {"plp",  in_plp },
#ifdef WDC
	[IN_PLX]  = {"plx",  in_plx },
#endif
#ifdef WDC
	[IN_PLY]  = {"ply",  in_ply },
#endif
#ifdef ROCKWEL
	[IN_RMB0] = {"rmb0", in_rmb0},
	[IN_RMB1] = {"rmb1", in_rmb1},
	[IN_RMB2] = {"rmb2", in_rmb2},
	[IN_RMB3] = {"rmb3", in_rmb3},
	[IN_RMB4] = {"rmb4", in_rmb4},
	[IN_RMB5] = {"rmb5", in_rmb5},
	[IN_RMB6] = {"rmb6", in_rmb6},
	[IN_RMB7] = {"rmb7", in_rmb7},
#endif
	[IN_ROL]  = {"rol",  in_rol },
	[IN_ROR]  = {"ror",  in_ror },
	[IN_RTI]  = {"rti",  in_rti },
	[IN_RTS]  = {"rts",  in_rts },
	[IN_SBC]  = {"sbc",  in_sbc },
	[IN_SEC]  = {"sec",  in_sec },
	[IN_SED]  = {"sed",  in_sed },
	[IN_SEI]  = {"sei",  in_sei },
#ifdef ROCKWEL
	[IN_SMB0] = {"smb0", in_smb0},
	[IN_SMB1] = {"smb1", in_smb1},
	[IN_SMB2] = {"smb2", in_smb2},
	[IN_SMB3] = {"smb3", in_smb3},
	[IN_SMB4] = {"smb4", in_smb4},
	[IN_SMB5] = {"smb5", in_smb5},
	[IN_SMB6] = {"smb6", in_smb6},
	[IN_SMB7] = {"smb7", in_smb7},
#endif
	[IN_STA]  = {"sta",  in_sta },
#ifdef WDC
	[IN_STP]  = {"stp",  in_stp },
#endif
	[IN_STX]  = {"stx",  in_stx },
	[IN_STY]  = {"sty",  in_sty },
#ifdef WDC
	[IN_STZ]  = {"stz",  in_stz },
#endif
	[IN_TAX]  = {"tax",  in_tax },
	[IN_TAY]  = {"tay",  in_tay },
#ifdef WDC
	[IN_TRB]  = {"trb",  in_trb },
#endif
#ifdef WDC
	[IN_TSB]  = {"tsb",  in_tsb },
#endif
	[IN_TSX]  = {"tsx",  in_tsx },
	[IN_TXA]  = {"txa",  in_txa },
	[IN_TXS]  = {"txs",  in_txs },
	[IN_TYA]  = {"tya",  in_tya },
#ifdef WDC
	[IN_WAI]  = {"wai",  in_wai },
#endif

#ifndef WDC
	[IN_XXX]  = {"",     in_xxx },
#endif
};

#pragma endregion data
