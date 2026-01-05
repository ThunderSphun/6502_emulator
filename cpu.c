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
// if they return true an additional clockcycle might need to be taken depending on the opcode


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

#pragma region opcodes
// the following functions all implement logic for the opcodes mode
// if they return true an additional clockcycle might need to be taken depending on the opcode

#if defined(WDC) || defined(ROCKWEL)
// some macro magic to get an easier time implementing the bit instructions
// these instrutions originated in the Rockwel chips, and were later adapted by WDC
#define BITS_EXPANSION(funcName) \
bool op_##funcName(uint8_t bit); \
BIT_EXPANSION(funcName, 0) BIT_EXPANSION(funcName, 1) \
BIT_EXPANSION(funcName, 2) BIT_EXPANSION(funcName, 3) BIT_EXPANSION(funcName, 4) \
BIT_EXPANSION(funcName, 5) BIT_EXPANSION(funcName, 6) BIT_EXPANSION(funcName, 7)
#define BIT_EXPANSION(funcName, bit) \
bool op_##funcName##bit() { return op_##funcName(bit); }
#endif


bool op_adc() {
	return false;
}

bool op_and() {
	return false;
}

bool op_asl() {
	return false;
}

#if defined(WDC) || defined(ROCKWEL)
bool op_bbr(uint8_t bit) {
	return false;
}

BITS_EXPANSION(bbr)
#endif

#if defined(WDC) || defined (ROCKWEL)
bool op_bbs(uint8_t bit) {
	return false;
}

BITS_EXPANSION(bbs)
#endif

bool op_bcc() {
	return false;
}

bool op_bcs() {
	return false;
}

bool op_beq() {
	return false;
}

bool op_bit() {
	return false;
}

bool op_bmi() {
	return false;
}

bool op_bne() {
	return false;
}

bool op_bpl() {
	return false;
}

#ifdef WDC
bool op_bra() {
	return false;
}
#endif

bool op_brk() {
	return false;
}

bool op_bvc() {
	return false;
}

bool op_bvs() {
	return false;
}

bool op_clc() {
	return false;
}

bool op_cld() {
	return false;
}

bool op_cli() {
	return false;
}

bool op_clv() {
	return false;
}

bool op_cmp() {
	return false;
}

bool op_cpx() {
	return false;
}

bool op_cpy() {
	return false;
}

bool op_dec() {
	return false;
}

bool op_dex() {
	return false;
}

bool op_dey() {
	return false;
}

bool op_eor() {
	return false;
}

bool op_inc() {
	return false;
}

bool op_inx() {
	return false;
}

bool op_iny() {
	return false;
}

bool op_jmp() {
	return false;
}

bool op_jsr() {
	return false;
}

bool op_lda() {
	return false;
}

bool op_ldx() {
	return false;
}

bool op_ldy() {
	return false;
}

bool op_lsr() {
	return false;
}

bool op_nop() {
	return false;
}

bool op_ora() {
	return false;
}

bool op_pha() {
	return false;
}

bool op_php() {
	return false;
}

#ifdef WDC
bool op_phx() {
	return false;
}
#endif

#ifdef WDC
bool op_phy() {
	return false;
}
#endif

bool op_pla() {
	return false;
}

bool op_plp() {
	return false;
}

#ifdef WDC
bool op_plx() {
	return false;
}
#endif

#ifdef WDC
bool op_ply() {
	return false;
}
#endif

#if defined(WDC) || defined (ROCKWEL)
bool op_rmb(uint8_t bit) {
	return false;
}

BITS_EXPANSION(rmb)
#endif

bool op_rol() {
	return false;
}

bool op_ror() {
	return false;
}

bool op_rti() {
	return false;
}

bool op_rts() {
	return false;
}

bool op_sbc() {
	return false;
}

bool op_sec() {
	return false;
}

bool op_sed() {
	return false;
}

bool op_sei() {
	return false;
}

#if defined(WDC) || defined (ROCKWEL)
bool op_smb(uint8_t bit) {
	return false;
}

BITS_EXPANSION(smb)
#endif

bool op_sta() {
	return false;
}

#ifdef WDC
bool op_stp() {
	return false;
}
#endif

bool op_stx() {
	return false;
}

bool op_sty() {
	return false;
}

#ifdef WDC
bool op_stz() {
	return false;
}
#endif

bool op_tax() {
	return false;
}

bool op_tay() {
	return false;
}

#ifdef WDC
bool op_trb() {
	return false;
}
#endif

#ifdef WDC
bool op_tsb() {
	return false;
}
#endif

bool op_tsx() {
	return false;
}

bool op_txa() {
	return false;
}

bool op_txs() {
	return false;
}

bool op_tya() {
	return false;
}

#ifdef WDC
bool op_wai() {
	return false;
}
#endif

#pragma endregion opcodes

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
