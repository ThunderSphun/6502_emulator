#include "cpu.h"

#include "bus.h"

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

size_t cycles = 0;

void push(uint8_t data) {
	bus_write(0x0100 | registers.SP--, data);
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

#pragma endregion addressingModes

#pragma region instructions
// the following functions all implement logic for the instructions
// if they return true an additional clockcycle might need to be taken depending on the addressing mode

#if defined(WDC) || defined(ROCKWEL)
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

#if defined(WDC) || defined(ROCKWEL)
// Branch on Bit Reset
// tests bit of accumulator, and branches if it is 0
// a branch taken takes an extra clock cycle
bool in_bbr(uint8_t bit) {
	return false;
}

BITS_EXPANSION(bbr)
#endif

#if defined(WDC) || defined (ROCKWEL)
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

#if defined(WDC) || defined (ROCKWEL)
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

#if defined(WDC) || defined (ROCKWEL)
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
// stops the clock from affecting the 6502
// the 6502 is faster to respond to the reset pin/function
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
// WAIt for interupt
// stops the clock from affecting the 6502
// the 6502 is faster to respond to the IRQ/NMI pins/functions
bool in_wai() {
	return false;
}
#endif

#pragma endregion instructions

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
