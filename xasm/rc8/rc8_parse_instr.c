/*  Copyright 2008-2026 Carsten Elton Sorensen and contributors

    This file is part of ASMotor.

    ASMotor is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ASMotor is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ASMotor.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include <stdbool.h>

#include "errors.h"
#include "expression.h"
#include "lexer.h"
#include "options.h"
#include "parse.h"
#include "parse_expression.h"
#include "section.h"

#include "rc8_errors.h"
#include "rc8_options.h"
#include "rc8_parse.h"
#include "rc8_tokens.h"

#define MODE_REG_F               0x00000001
#define MODE_REG_T               0x00000002
#define MODE_REG_B               0x00000004
#define MODE_REG_C               0x00000008
#define MODE_REG_D               0x00000010
#define MODE_REG_E               0x00000020
#define MODE_REG_H               0x00000040
#define MODE_REG_L               0x00000080
#define MODE_REG_FT              0x00000100
#define MODE_REG_BC              0x00000200
#define MODE_REG_DE              0x00000400
#define MODE_REG_HL              0x00000800
#define MODE_IND_FT              0x00001000
#define MODE_IND_BC              0x00002000
#define MODE_IND_DE              0x00004000
#define MODE_IND_HL              0x00008000
#define MODE_IND_FT_POST_INC     0x00010000
#define MODE_IND_BCDEHL_POST_INC 0x00020000
#define MODE_IND_FT_PRE_DEC      0x00040000
#define MODE_IND_BCDEHL_PRE_DEC  0x00080000
#define MODE_IND_C               0x00100000
#define MODE_IMM                 0x00200000
#define MODE_ADDR                0x00400000
#define MODE_REGISTER_MASK       0x00800000
#define MODE_NONE                0x01000000

#define MODE_REG_8BIT                 (MODE_REG_F | MODE_REG_T | MODE_REG_B | MODE_REG_C | MODE_REG_D | MODE_REG_E | MODE_REG_H | MODE_REG_L)
#define MODE_REG_8BIT_FBCDEHL         (MODE_REG_F | MODE_REG_B | MODE_REG_C | MODE_REG_D | MODE_REG_E | MODE_REG_H | MODE_REG_L)
#define MODE_REG_8BIT_BCDEHL          (MODE_REG_B | MODE_REG_C | MODE_REG_D | MODE_REG_E | MODE_REG_H | MODE_REG_L)
#define MODE_REG_16BIT                (MODE_REG_FT | MODE_REG_BC | MODE_REG_DE | MODE_REG_HL)
#define MODE_REG_16BIT_BCDEHL         (MODE_REG_BC | MODE_REG_DE | MODE_REG_HL)
#define MODE_IND_16BIT                (MODE_IND_FT | MODE_IND_BC | MODE_IND_DE | MODE_IND_HL)
#define MODE_IND_16BIT_BCDEHL         (MODE_IND_BC | MODE_IND_DE | MODE_IND_HL)
#define MODE_IND_16BIT_INC_DEC        (MODE_IND_FT_POST_INC | MODE_IND_BCDEHL_POST_INC | MODE_IND_FT_PRE_DEC | MODE_IND_BCDEHL_PRE_DEC)
#define MODE_IND_16BIT_FT_INC_DEC     (MODE_IND_FT_POST_INC | MODE_IND_FT_PRE_DEC)
#define MODE_IND_16BIT_BCDEHL_INC_DEC (MODE_IND_BCDEHL_POST_INC | MODE_IND_BCDEHL_PRE_DEC)

typedef enum {
	// These must be in the same order as the tokens and end with CC_ALWAYS
	// as this is the order of the opcodes
	CC_LE,
	CC_GT,
	CC_LT,
	CC_GE,
	CC_LEU,
	CC_GTU,
	CC_LTU,
	CC_GEU,
	CC_EQ,
	CC_NE,
	CC_ALWAYS,
} EConditionCode;

typedef enum {
	CONDITION_AUTO,
	CONDITION_PASS
} EConditionType;

typedef struct {
	uint32_t mode;
	uint32_t registerIndex;
	SExpression* expression;
} SAddressingMode;

typedef enum {
	ALU_ADD,
	ALU_SUB,
	ALU_AND,
	ALU_OR,
	ALU_XOR,
	ALU_CMP
} EAluOp;

static bool
handle_LD(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source);
static bool
handle_LD_R8_const(SAddressingMode* reg, uint8_t n8);
static bool
handle_SynthRegOp(EAluOp op, uint8_t base8, uint8_t base16, EConditionCode cc, SAddressingMode* destination,
                  SAddressingMode* source);

static void
addrMode_FreeAll(SAddressingMode* addrMode) {
	expr_Free(addrMode->expression);
	addrMode->expression = NULL;
}

typedef struct Parser {
	uint8_t baseOpcode;
	EConditionType condition;
	uint32_t firstModes;
	uint32_t secondModes;
	bool (*parser)(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* mode1, SAddressingMode* mode2);
} SParser;

static string*
createUniqueLabel(void) {
	static uint32_t id = 0;
	char sym[32];

	snprintf(sym, sizeof(sym), "$target%u", id++);
	return str_Create(sym);
}

static EConditionCode
invertCondition(EConditionCode cc) {
	if (cc == CC_ALWAYS)
		return CC_ALWAYS;

	return cc ^ 1;
}

static bool
handle_OpcodeRegister(uint8_t baseOpcode, SAddressingMode* sourceMode) {
	sect_OutputConst8(baseOpcode | sourceMode->registerIndex);
	return true;
}

static bool
handle_Op_Reg_Imm(uint8_t baseOpcode, int lowerBound, int upperBound, SAddressingMode* registerDest, SExpression* expression) {
	SExpression* ranged = expr_CheckRange(expression, lowerBound, upperBound);
	if (ranged == NULL) {
		return err_Error(ERROR_OPERAND_RANGE);
	}

	SExpression* masked = //
	    expr_And(         //
	        ranged,       //
	        expr_Const(0xFF));

	if (masked == NULL) {
		return false;
	}

	if (registerDest == NULL)
		sect_OutputConst8(baseOpcode);
	else
		handle_OpcodeRegister(baseOpcode, registerDest);

	sect_OutputExpr8(masked);

	return true;
}

static void
registerPair(SAddressingMode* high, SAddressingMode* low, SAddressingMode* src) {
	uint32_t registerBase = src->registerIndex * 2;
	high->mode = MODE_REG_F << registerBase;
	high->registerIndex = registerBase;
	low->mode = MODE_REG_T << registerBase;
	low->registerIndex = registerBase + 1;
}

static bool
handle_Op_Reg_SignedImm(uint8_t baseOpcode, SAddressingMode* registerDest, SExpression* expression) {
	return handle_Op_Reg_Imm(baseOpcode, -128, 127, registerDest, expression);
}

static bool
handle_Op_Reg_SignedOrUnsignedImm(uint8_t baseOpcode, SAddressingMode* registerDest, SExpression* expression) {
	return handle_Op_Reg_Imm(baseOpcode, -128, 255, registerDest, expression);
}

static bool
handle_Op_Reg_UnsignedImm(uint8_t baseOpcode, SAddressingMode* registerDest, SExpression* expression) {
	return handle_Op_Reg_Imm(baseOpcode, 0, 255, registerDest, expression);
}

static void
ensureSource(SAddressingMode** destination, SAddressingMode** source) {
	// If only destination has been specified, swap the two modes.
	// This is usually used with instruction that have an implicit destination, such as T

	if ((*source)->mode == MODE_NONE) {
		SAddressingMode* t = *destination;
		*destination = *source;
		*source = t;
	}
}

// Native byte accesses through the FT pointer.  `r8` is the byte-register index
// (B=2 .. L=7); F and T cannot name the native stepped operand.  `p` is the
// 16-bit pair index (BC=1 .. HL=3).  These emit the hardware stepped-byte
// opcodes ($12-$17 store, $1A-$1F load) and EXG FT,r16 ($C9-$CB) directly.
static void
out_LD_r8_FT(uint8_t r8) { sect_OutputConst8(0x28 | r8); }      // LD r8,(FT)
static void
out_LD_r8_FTinc(uint8_t r8) { sect_OutputConst8(0x18 | r8); }   // LD r8,(FT+)
static void
out_LDmemFT_r8(uint8_t r8) { sect_OutputConst8(0x20 | r8); }    // LD (FT),r8
static void
out_LDmemFTinc_r8(uint8_t r8) { sect_OutputConst8(0x10 | r8); } // LD (FT+),r8
static void
out_EXG_FT_r16(uint8_t p) { sect_OutputConst8(0xC8 | p); }      // EXG FT,r16
static void
out_ADD_FT(int8_t n) { sect_OutputConst8(0x9C); sect_OutputConst8((uint8_t)n); }
static void
out_ADD_r16(uint8_t p, int8_t n) { sect_OutputConst8(0x9C | p); sect_OutputConst8((uint8_t)n); }

// Word load/store through the FT pointer: FT is the address, the data is a
// BC/DE/HL pair given as its low/high byte-register indices (little-endian: low
// first).  All use the hardware stepped-byte opcodes; `step` is 0 (plain), +2
// (post-inc) or -2 (pre-dec) and is the amount FT ends up advanced by.
static void
ft_load_pair(uint8_t lo, uint8_t hi, int8_t step) {
	if (step == -2)
		out_ADD_FT(-2);
	if (step == 2) {
		out_LD_r8_FTinc(lo); // low  = [FT],   FT += 1
		out_LD_r8_FTinc(hi); // high = [FT+1], FT += 1
	} else {
		out_LD_r8_FTinc(lo); // low  = [FT], FT += 1
		out_LD_r8_FT(hi);    // high = [FT+1]
		out_ADD_FT(-1);      // land FT back at base (plain) / base-2 (pre-dec)
	}
}
static void
ft_store_pair(uint8_t lo, uint8_t hi, int8_t step) {
	if (step == -2)
		out_ADD_FT(-2);
	if (step == 2) {
		out_LDmemFTinc_r8(lo); // [FT]   = low,  FT += 1
		out_LDmemFTinc_r8(hi); // [FT+1] = high, FT += 1
	} else {
		out_LDmemFTinc_r8(lo); // [FT]   = low,  FT += 1
		out_LDmemFT_r8(hi);    // [FT+1] = high
		out_ADD_FT(-1);
	}
}

// Word load/store through a BC/DE/HL pair pointer: the pair is the address and
// FT is the data.  `exg ft,p` lends the pointer to FT so the native stepped
// bytes can run through it, then `exg ft,p` returns the stepped pointer.  On a
// load FT is the destination (its prior value is scratch); on a store FT is the
// source value and rides in `p` across the swap, so it survives.
static void
pair_load_ft(uint8_t p, uint8_t lo, uint8_t hi, int8_t step) {
	if (step == -2)
		out_ADD_r16(p, -2);
	out_EXG_FT_r16(p); // FT = pointer, p = old FT
	if (step == 2) {
		out_LD_r8_FTinc(lo);
		out_LD_r8_FTinc(hi);
	} else {
		out_LD_r8_FTinc(lo);
		out_LD_r8_FT(hi);
		out_ADD_FT(-1);
	}
	out_EXG_FT_r16(p); // FT = loaded word, p = stepped pointer
}
static void
pair_store_ft(uint8_t p, uint8_t lo, uint8_t hi, int8_t step) {
	if (step == -2)
		out_ADD_r16(p, -2);
	out_EXG_FT_r16(p);
	if (step == 2) {
		out_LDmemFTinc_r8(lo);
		out_LDmemFTinc_r8(hi);
	} else {
		out_LDmemFTinc_r8(lo);
		out_LDmemFT_r8(hi);
		out_ADD_FT(-1);
	}
	out_EXG_FT_r16(p); // FT restored to its source value, p = stepped pointer
}

// Word I/O through a BC/DE/HL pair pointer: there is no native stepped I/O
// opcode, so the word is two single-byte I/O accesses through the pair pointer
// `*ptr`, low byte first, with explicit ADDs to walk it.  `io` is the byte I/O
// base (LD T,(p) / LD (p),T form); `step` is 0 / +2 / -2.  A store preserves FT
// via EXG F,T.
static void
pair_io_ft(SAddressingMode* ptr, uint8_t io, bool isStore, int8_t step) {
	if (step == -2) {
		handle_OpcodeRegister(0x9C, ptr);
		sect_OutputConst8(-2);
	}
	handle_OpcodeRegister(io, ptr); // low  = [p] (or [p] = low)
	handle_OpcodeRegister(0x9C, ptr);
	sect_OutputConst8(1);
	sect_OutputConst8(0x10); // EXG F,T
	handle_OpcodeRegister(io, ptr); // high = [p+1] (or [p+1] = high)
	if (step == 0) {
		handle_OpcodeRegister(0x9C, ptr);
		sect_OutputConst8(-1);
	} else if (step == -2) {
		handle_OpcodeRegister(0x9C, ptr);
		sect_OutputConst8(-1); // land p at orig-2
	} else {
		handle_OpcodeRegister(0x9C, ptr);
		sect_OutputConst8(1); // post-inc: p ends at orig+2
	}
	sect_OutputConst8(0x10); // EXG F,T (restore FT on a store)
	(void)isStore;
}

static bool
handle_LongLoad(uint8_t ld_t_hl, uint8_t ld_hl_t, uint8_t ld_r_ft, uint8_t ld_ft_r, EConditionCode cc, bool isLD,
                SAddressingMode* destination, SAddressingMode* source) {
	if (!opt_Current->machineOptions->enableSynthInstructions)
		return false;

	if (ld_t_hl != 0xFF && destination->mode == MODE_REG_FT && (source->mode & MODE_IND_16BIT_BCDEHL)) {
		// LD FT,(hl)  — pair pointer hl, FT data, step 0
		if (isLD) {
			SAddressingMode high, low;
			registerPair(&high, &low, source);
			pair_load_ft(source->registerIndex, low.registerIndex, high.registerIndex, 0);
		} else
			pair_io_ft(source, ld_t_hl, false, 0);
		return true;
	} else if (ld_hl_t != 0xFF && (destination->mode & MODE_IND_16BIT_BCDEHL) && source->mode == MODE_REG_FT) {
		// LD (hl),FT  — pair pointer hl, FT data, step 0
		if (isLD) {
			SAddressingMode high, low;
			registerPair(&high, &low, destination);
			pair_store_ft(destination->registerIndex, low.registerIndex, high.registerIndex, 0);
		} else
			pair_io_ft(destination, ld_hl_t, true, 0);
		return true;
	} else if (ld_t_hl != 0xFF && destination->mode == MODE_REG_FT && (source->mode & MODE_IND_BCDEHL_POST_INC)) {
		// LD FT,(hl+)  — pair pointer hl, FT data, step +2
		if (isLD) {
			SAddressingMode high, low;
			registerPair(&high, &low, source);
			pair_load_ft(source->registerIndex, low.registerIndex, high.registerIndex, 2);
		} else
			pair_io_ft(source, ld_t_hl, false, 2);
		return true;
	} else if (ld_t_hl != 0xFF && destination->mode == MODE_REG_T && (source->mode & MODE_IND_BCDEHL_POST_INC)) {
		// LD T,(hl+)
		handle_OpcodeRegister(ld_t_hl, source); // LD T,(hl)
		handle_OpcodeRegister(0x9C, source);    // ADD hl,
		sect_OutputConst8(1);                   //        1
		return true;
	} else if (ld_hl_t != 0xFF && (destination->mode & MODE_IND_BCDEHL_POST_INC) && source->mode == MODE_REG_FT) {
		// LD (hl+),FT  — pair pointer hl, FT data, step +2
		if (isLD) {
			SAddressingMode high, low;
			registerPair(&high, &low, destination);
			pair_store_ft(destination->registerIndex, low.registerIndex, high.registerIndex, 2);
		} else
			pair_io_ft(destination, ld_hl_t, true, 2);
		return true;
	} else if (ld_hl_t != 0xFF && (destination->mode & MODE_IND_BCDEHL_POST_INC) && source->mode == MODE_REG_T) {
		// LD (hl+),T  — byte store through a pair pointer, step +1 (= width)
		handle_OpcodeRegister(ld_hl_t, destination); // LD (hl),T
		handle_OpcodeRegister(0x9C, destination);    // ADD hl,
		sect_OutputConst8(1);                        //        1
		return true;
	} else if (ld_t_hl != 0xFF && destination->mode == MODE_REG_FT && (source->mode & MODE_IND_BCDEHL_PRE_DEC)) {
		// LD FT,(-hl)  — pair pointer hl, FT data, step -2
		if (isLD) {
			SAddressingMode high, low;
			registerPair(&high, &low, source);
			pair_load_ft(source->registerIndex, low.registerIndex, high.registerIndex, -2);
		} else
			pair_io_ft(source, ld_t_hl, false, -2);
		return true;
	} else if (ld_t_hl != 0xFF && destination->mode == MODE_REG_T && (source->mode & MODE_IND_BCDEHL_PRE_DEC)) {
		// LD T,(-hl)
		handle_OpcodeRegister(0x9C, source);    // ADD hl,
		sect_OutputConst8(-1);                  //        -1
		handle_OpcodeRegister(ld_t_hl, source); // LD T,(hl)
		return true;
	} else if (ld_hl_t != 0xFF && (destination->mode & MODE_IND_BCDEHL_PRE_DEC) && source->mode == MODE_REG_FT) {
		// LD (-hl),FT  — pair pointer hl, FT data, step -2
		if (isLD) {
			SAddressingMode high, low;
			registerPair(&high, &low, destination);
			pair_store_ft(destination->registerIndex, low.registerIndex, high.registerIndex, -2);
		} else
			pair_io_ft(destination, ld_hl_t, true, -2);
		return true;
	} else if (ld_hl_t != 0xFF && (destination->mode & MODE_IND_BCDEHL_PRE_DEC) && source->mode == MODE_REG_T) {
		// LD (-hl),T
		handle_OpcodeRegister(0x9C, destination);    // ADD hl,
		sect_OutputConst8(-1);                       //        -1
		handle_OpcodeRegister(ld_hl_t, destination); // LD (hl),T
		return true;
	} else if (ld_r_ft != 0xFF && (destination->mode & MODE_REG_16BIT_BCDEHL) && source->mode == MODE_IND_FT) {
		// LD hl,(FT)  — FT pointer, pair data, step 0
		SAddressingMode high, low;
		registerPair(&high, &low, destination);
		ft_load_pair(low.registerIndex, high.registerIndex, 0);
		return true;
	} else if (ld_ft_r != 0xFF && destination->mode == MODE_IND_FT && (source->mode & MODE_REG_16BIT_BCDEHL)) {
		// LD (FT),hl  — FT pointer, pair data, step 0
		SAddressingMode high, low;
		registerPair(&high, &low, source);
		ft_store_pair(low.registerIndex, high.registerIndex, 0);
		return true;
	} else if (ld_r_ft != 0xFF && (destination->mode & MODE_REG_16BIT_BCDEHL) && source->mode == MODE_IND_FT_POST_INC) {
		// LD hl,(FT+)  — FT pointer, pair data, step +2
		SAddressingMode high, low;
		registerPair(&high, &low, destination);
		ft_load_pair(low.registerIndex, high.registerIndex, 2);
		return true;
	} else if (ld_r_ft != 0xFF && (destination->mode & MODE_REG_8BIT_FBCDEHL) && source->mode == MODE_IND_FT_POST_INC) {
		// LD r,(FT+)  — byte load through FT, step +1 (= width)
		uint8_t ri = destination->registerIndex;
		if (ri == 0)
			return false; // LD F,(FT+) clobbers the FT pointer it steps: nonsense
		out_LD_r8_FTinc(ri);
		return true;
	} else if (ld_ft_r != 0xFF && destination->mode == MODE_IND_FT_POST_INC && (source->mode & MODE_REG_16BIT_BCDEHL)) {
		// LD (FT+),hl  — FT pointer, pair data, step +2
		SAddressingMode high, low;
		registerPair(&high, &low, source);
		ft_store_pair(low.registerIndex, high.registerIndex, 2);
		return true;
	} else if (ld_ft_r != 0xFF && destination->mode == MODE_IND_FT_POST_INC && (source->mode & MODE_REG_8BIT_FBCDEHL)) {
		// LD (FT+),r  — byte store through FT, step +1 (= width)
		uint8_t ri = source->registerIndex;
		if (ri == 0)
			return false; // LD (FT+),F clobbers the FT pointer it steps: nonsense
		out_LDmemFTinc_r8(ri);
		return true;
	} else if (ld_r_ft != 0xFF && (destination->mode & MODE_REG_16BIT_BCDEHL) && source->mode == MODE_IND_FT_PRE_DEC) {
		// LD hl,(-FT)  — FT pointer, pair data, step -2
		SAddressingMode high, low;
		registerPair(&high, &low, destination);
		ft_load_pair(low.registerIndex, high.registerIndex, -2);
		return true;
	} else if (ld_r_ft != 0xFF && (destination->mode & MODE_REG_8BIT_FBCDEHL) && source->mode == MODE_IND_FT_PRE_DEC) {
		// LD r,(-FT)  — byte load through FT, step -1 (= width); no native pre-dec
		if (destination->registerIndex == 0)
			return false; // LD F,(-FT) clobbers the FT pointer it steps: nonsense
		handle_OpcodeRegister(0x9C, source);         // ADD FT,
		sect_OutputConst8(-1);                       //        -1
		handle_OpcodeRegister(ld_r_ft, destination); // LD r,(FT)
		return true;
	} else if (ld_ft_r != 0xFF && destination->mode == MODE_IND_FT_PRE_DEC && (source->mode & MODE_REG_16BIT_BCDEHL)) {
		// LD (-FT),hl  — FT pointer, pair data, step -2
		SAddressingMode high, low;
		registerPair(&high, &low, source);
		ft_store_pair(low.registerIndex, high.registerIndex, -2);
		return true;
	} else if (ld_ft_r != 0xFF && destination->mode == MODE_IND_FT_PRE_DEC && (source->mode & MODE_REG_8BIT_FBCDEHL)) {
		// LD (-FT),r  — byte store through FT, step -1 (= width); no native pre-dec
		if (source->registerIndex == 0)
			return false; // LD (-FT),F clobbers the FT pointer it steps: nonsense
		handle_OpcodeRegister(0x9C, destination); // ADD FT,
		sect_OutputConst8(-1);                    //        -1
		handle_OpcodeRegister(ld_ft_r, source);   // LD (FT),r
		return true;
	}

	return false;
}

static bool
handle_Shift(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	ensureSource(&destination, &source);

	if ((destination->mode & (MODE_REG_FT | MODE_NONE)) && (source->mode & MODE_REG_8BIT_BCDEHL))
		return handle_OpcodeRegister(baseOpcode, source);
	else if ((destination->mode & (MODE_REG_FT | MODE_NONE)) && (source->mode & MODE_IMM)) {
		if (expr_IsConstant(source->expression) && (source->expression->value.integer < 0 || source->expression->value.integer > 15)) {
			expr_Free(source->expression);
			source->expression = NULL;
			return err_Error(MERROR_SHIFT_COUNT_RANGE);
		}
		SExpression* ranged = expr_CheckRange(source->expression, 0, 15);
		source->expression = NULL;
		if (ranged == NULL)
			return false;
		/* The immediate block mirrors the register-form rows: the shift's
		 two row bits (4-3) select the slot, so RS ($E0) is $B8, RSA ($E8)
		 is $B9 and LS ($F0) is $BA. $BB is the hole left by the shuffle. */
		baseOpcode = 0xB8 + ((baseOpcode >> 3) & 0x03);
		sect_OutputConst8(baseOpcode);
		sect_OutputExpr8(ranged);
		return true;
	}

	return false;
}

// The byte-register exchange through the hub (`EXG T,r8`, `$12-$17`) is gone
// from the ISA, so the only byte exchange left to synthesize with is `EXG F,T`
// ($10): a byte operation whose destination is not F has no exchange to open
// with, and must be spelled by hand as moves (`ld t,b` / `add t,e` / `ld b,t`)
// or as a pair exchange (`exg ft,bc` / `add t,c` / `exg ft,bc`). Pair
// destinations are unaffected — they exchange through `EXG FT,r16` ($C9-$CB).
static bool
byteExchangeAvailable(const SAddressingMode* destination) {
	return (destination->mode & (MODE_REG_16BIT | MODE_REG_F)) != 0;
}

// Expand `OP X,X` — both operands the same register — into the minimal
// native sequence. The caller has checked that synthesis is enabled.
static bool
handle_SynthSelfOp(EAluOp op, EConditionCode cc, SAddressingMode* destination) {
	switch (op) {
		case ALU_CMP:
			// X - X is statically zero-equal: write FLAGS_EQ straight to F.
			err_Warn(MERROR_CMP_SELF_ALWAYS);
			{
				SAddressingMode f = {MODE_REG_F, 0, NULL};
				handle_LD_R8_const(&f, 0x02); // LD F,FLAGS_EQ
			}
			return true;

		case ALU_SUB:
		case ALU_XOR: {
			// X - X and X ^ X are zero: LD X,0 (synthesized for R16).
			SAddressingMode zero = {MODE_IMM, 0, expr_Const(0)};
			return handle_LD(0, cc, destination, &zero);
		}

		case ALU_AND:
		case ALU_OR:
			// X & X and X | X keep X — nothing to emit.
			err_Warn(MERROR_BITWISE_SELF_NOOP);
			return true;

		case ALU_ADD: {
			// X + X doubles X: route through the hub and shift once.
			bool is16 = (destination->mode & MODE_REG_16BIT) != 0;
			uint8_t exgBase = is16 ? 0xC8 : 0x10;
			bool result;

			if (!byteExchangeAvailable(destination))
				return false; // `add b,b` has no byte exchange left to open with

			handle_OpcodeRegister(exgBase, destination); // EXG X,HUB
			if (is16) {
				SAddressingMode ft = {MODE_REG_FT, 0, NULL};
				SAddressingMode one = {MODE_IMM, 0, expr_Const(1)};
				result = handle_Shift(0xF0, cc, &ft, &one); // LS FT,1
			} else {
				SAddressingMode t = {MODE_REG_T, 1, NULL};
				result = handle_OpcodeRegister(0x40, &t); // ADD T,T
			}
			handle_OpcodeRegister(exgBase, destination); // EXG X,HUB
			return result;
		}
	}

	return false;
}

// Synthesize `OP DST,SRC` for register operands routed through the T/FT
// hub: EXG DST,HUB / OP HUB,SRC' / EXG DST,HUB. The exchanges restore DST
// and the hub register afterwards; only CMP writes F, and the byte exchange
// never touches F, so comparison flags survive exactly where they are
// intended. That leaves the byte forms working only when DST is F — the
// exchange is then `EXG F,T` ($10) — because the byte exchange through T
// (`$12-$17`) is out of the ISA. SRC' remaps the hub operand to DST, which
// holds the pre-exchange hub value after the first exchange. Self-operations
// take a direct expansion instead. Returns false for combinations that stay
// an operand error (width mismatch, unimplemented forms).
static bool
handle_SynthRegOp(EAluOp op, uint8_t base8, uint8_t base16, EConditionCode cc, SAddressingMode* destination,
                  SAddressingMode* source) {
	bool is16 = (destination->mode & MODE_REG_16BIT) != 0;
	bool src8 = (source->mode & MODE_REG_8BIT) != 0;
	bool src16 = (source->mode & MODE_REG_16BIT) != 0;

	if (is16 ? !src16 : !src8)
		return false; // width mismatch remains an operand error

	// Byte destinations other than F have had no exchange to open with since
	// `$12-$17` left the ISA, so this is an operand error whether or not
	// synthesis is on. Self-operations are exempt where they need no exchange
	// (`cmp b,b` writes the equal flags, `xor b,b` vanishes); doubling is the
	// one that does, and it stays an error.
	if (!byteExchangeAvailable(destination) &&
	    !(destination->mode == source->mode && op != ALU_ADD))
		return false;

	if (!opt_Current->machineOptions->enableSynthInstructions)
		return err_Error(MERROR_REQUIRES_SYNTHESIZED);

	if (destination->mode == source->mode)
		return handle_SynthSelfOp(op, cc, destination);

	SAddressingMode src = *source;
	uint8_t exgBase = is16 ? 0xC8 : 0x10;

	if (is16 ? (src.mode == MODE_REG_FT) : (src.mode == MODE_REG_T))
		src = *destination;

	if (op == ALU_CMP && destination->mode == MODE_REG_F) {
		// The destination is the flags register itself: the closing
		// EXG F,T would carry the fresh flags out of F, so the exchange
		// stays open. T ends up holding the old F value (documented).
		handle_OpcodeRegister(0x10, destination); // EXG F,T
		return handle_OpcodeRegister(base8, &src);
	}

	return handle_OpcodeRegister(exgBase, destination) && handle_OpcodeRegister(is16 ? base16 : base8, &src) &&
	       handle_OpcodeRegister(exgBase, destination);
}

static bool
handle_ADD(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	ensureSource(&destination, &source);

	if ((destination->mode & (MODE_REG_T | MODE_NONE)) && (source->mode & MODE_REG_8BIT))
		return handle_OpcodeRegister(0x40, source);
	else if ((destination->mode & (MODE_REG_FT | MODE_NONE)) && (source->mode & MODE_REG_16BIT)) {
		if (source->mode == MODE_REG_FT) {
			// ADD FT,FT (or the implicit ADD FT) doubles FT. Emit the
			// equivalent LS FT,1 instead of the redundant native form.
			if (!opt_Current->machineOptions->enableSynthInstructions)
				return err_Error(MERROR_REQUIRES_SYNTHESIZED);
			SAddressingMode one = {MODE_IMM, 0, expr_Const(1)};
			return handle_Shift(0xF0, cc, destination, &one);
		}
		return handle_OpcodeRegister(0xFC, source);
	}
	else if ((destination->mode & MODE_REG_8BIT) && (source->mode & MODE_IMM)) {
		bool r = handle_Op_Reg_SignedOrUnsignedImm(0xA0, destination, source->expression);
		source->expression = NULL;
		return r;
	}
	else if ((destination->mode & MODE_REG_16BIT) && (source->mode & MODE_IMM)) {
		SExpression* ranged = expr_CheckRange(source->expression, -32768, 65535);
		source->expression = NULL;
		if (ranged == NULL) {
			return err_Error(ERROR_OPERAND_RANGE);
		}

		if (expr_IsConstant(ranged) && ranged->value.integer >= -128 && ranged->value.integer <= 127) {
			return handle_Op_Reg_SignedImm(0x9C, destination, ranged);
		}

		SExpression* expr = expr_Asr(expr_Asl(ranged, expr_Const(16)), expr_Const(16));

		if (expr_IsConstant(expr) && expr->value.integer >= -128 && expr->value.integer <= 127) {
			return handle_Op_Reg_SignedImm(0x9C, destination, expr);
		}

		if (!opt_Current->machineOptions->enableSynthInstructions) {
			expr_Free(expr);
			return err_Error(MERROR_REQUIRES_SYNTHESIZED);
		}

		SAddressingMode highAddr, lowAddr;
		registerPair(&highAddr, &lowAddr, destination);

		SExpression* low = expr_Asr(expr_Asl(expr_Clone(expr), expr_Const(24)), expr_Const(24));
		SExpression* highAdjust = expr_Asr(expr_Clone(low), expr_Const(8));
		SExpression* high = expr_Sub(expr_Asr(expr_Clone(expr), expr_Const(8)), highAdjust);
		expr_Free(expr);

		if (!expr_IsConstant(low) || low->value.integer != 0) {
			handle_Op_Reg_SignedImm(0x9C, destination, low);
		} else {
			expr_Free(low);
		}
		if (!expr_IsConstant(high) || high->value.integer != 0) {
			handle_Op_Reg_SignedImm(0xA0, &highAddr, high);
		} else {
			expr_Free(high);
		}

		return true;
	} else if ((destination->mode & (MODE_REG_8BIT & ~MODE_REG_T)) && (source->mode & MODE_REG_8BIT))
		return handle_SynthRegOp(ALU_ADD, 0x40, 0xFC, cc, destination, source);
	else if ((destination->mode & (MODE_REG_16BIT & ~MODE_REG_FT)) && (source->mode & MODE_REG_16BIT))
		return handle_SynthRegOp(ALU_ADD, 0x40, 0xFC, cc, destination, source);

	return false;
}

static bool
handle_Bitwise(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	ensureSource(&destination, &source);

	if ((source->mode & MODE_REG_8BIT_FBCDEHL) && (destination->mode & (MODE_NONE | MODE_REG_T)))
		return handle_OpcodeRegister(baseOpcode, source);
	else if ((source->mode & MODE_IMM) && (destination->mode & (MODE_NONE | MODE_REG_T))) {
		if (expr_IsConstant(source->expression) && source->expression->value.integer >= 0 && source->expression->value.integer <= 255) {
			if ((baseOpcode == 0x60 || baseOpcode == 0x70) && source->expression->value.integer == 0)
				err_Warn(MERROR_BITWISE_ZERO_NOOP);
			else if (baseOpcode == 0x68 && source->expression->value.integer == 255)
				err_Warn(MERROR_BITWISE_ZERO_NOOP);
		}
		baseOpcode = 0xB0 + ((baseOpcode >> 3) & 0x03);
		bool r = handle_Op_Reg_SignedOrUnsignedImm(baseOpcode, NULL, source->expression);
		source->expression = NULL;
		return r;
	} else if ((destination->mode & MODE_REG_8BIT) && (source->mode & MODE_REG_8BIT)) {
		EAluOp op = (baseOpcode == 0x68) ? ALU_AND : (baseOpcode == 0x60) ? ALU_OR : ALU_XOR;
		return handle_SynthRegOp(op, baseOpcode, 0, cc, destination, source);
	}

	return false;
}

static bool
handle_EXG(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	ensureSource(&destination, &source);

	// The byte exchange survives only as `EXG F,T` ($10): `EXG T,r8`
	// (`$12-$17`) is out of the ISA, so `exg b`, `exg b,c` and friends are
	// operand errors and the three-exchange byte thunk is gone.
	if ((destination->mode & MODE_REG_F) && (source->mode & MODE_REG_T)) {
		return handle_OpcodeRegister(baseOpcode, destination);
	} else if ((source->mode & MODE_REG_F) && (destination->mode & (MODE_NONE | MODE_REG_T))) {
		return handle_OpcodeRegister(baseOpcode, source);
	} else if ((destination->mode & MODE_REG_16BIT_BCDEHL) && (source->mode & MODE_REG_FT)) {
		return handle_OpcodeRegister(0xC8, destination);
	} else if ((source->mode & MODE_REG_16BIT_BCDEHL) && (destination->mode & (MODE_NONE | MODE_REG_FT))) {
		return handle_OpcodeRegister(0xC8, source);
	} else if ((source->mode & MODE_REG_16BIT_BCDEHL) && (destination->mode & MODE_REG_16BIT_BCDEHL)) {
		if (!opt_Current->machineOptions->enableSynthInstructions)
			return err_Error(MERROR_REQUIRES_SYNTHESIZED);

		return handle_OpcodeRegister(0xC8, source) && handle_OpcodeRegister(0xC8, destination) &&
		       handle_OpcodeRegister(0xC8, source);
	}

	return false;
}

static bool
handle_Implicit(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	sect_OutputConst8(baseOpcode);
	return true;
}

static bool
handle_JumpRelative(uint8_t opcode, SExpression* destination) {
	sect_OutputConst8(opcode);

	SExpression* pcrelExpr = expr_CheckRange(expr_PcRelative(destination, 0), -128, 127);
	if (pcrelExpr == NULL)
		return false;

	sect_OutputExpr8(pcrelExpr);
	return true;
}

static bool
handle_JumpRelativeCC(EConditionCode cc, SExpression* destination) {
	return handle_JumpRelative(0x90 + cc, destination);
}

static bool
handle_DJ(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	bool r = handle_JumpRelative(baseOpcode + destination->registerIndex, source->expression);
	source->expression = NULL;
	return r;
}

static bool
handle_J(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	if (destination->mode == MODE_ADDR && source->mode == MODE_NONE) {
		bool r = handle_JumpRelativeCC(cc, destination->expression);
		destination->expression = NULL;
		return r;
	}

	if ((destination->mode & MODE_IND_16BIT) && source->mode == MODE_NONE) {
		if (cc != CC_ALWAYS) {
			if (!opt_Current->machineOptions->enableSynthInstructions)
				return err_Error(MERROR_REQUIRES_SYNTHESIZED);

			cc = invertCondition(cc);
			sect_OutputConst8(0x90 + cc);
			sect_OutputConst8(0x02);
		}

		return handle_OpcodeRegister(baseOpcode, destination);
	}

	return false;
}

static bool
handle_LCO(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	if ((source->mode & MODE_IND_16BIT_INC_DEC) ||
	    (destination->mode == MODE_REG_FT && (source->mode & MODE_IND_16BIT_BCDEHL)))
		return handle_LongLoad(0x0C, 0xFF, 0xFF, 0xFF, cc, false, destination, source);
	else if (destination->mode == MODE_REG_T)
		return handle_OpcodeRegister(baseOpcode, source);

	return false;
}

static bool
handle_LCR(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	if (destination->mode == MODE_IND_C && source->mode == MODE_REG_T)
		return handle_Implicit(baseOpcode, cc, destination, source);
	else if (destination->mode == MODE_REG_T && source->mode == MODE_IND_C)
		return handle_Implicit(baseOpcode + 0x01, cc, destination, source);

	return false;
}

static bool
handle_LD_R8_const(SAddressingMode* reg, uint8_t n8) {
	if (!handle_OpcodeRegister(0x80, reg))
		return false;

	sect_OutputConst8(n8);
	return true;
}

static bool
handle_LD_R16_3bytes(SAddressingMode* registerHigh, SAddressingMode* registerLow, uint8_t high, uint8_t low) {
	if (registerHigh->registerIndex == 0 && registerLow->registerIndex == 1) {
		if (((high == 0xFF) && (low & 0x80)) || ((high == 0x00) && (low & 0x80) == 0)) {
			handle_LD_R8_const(registerLow, low); /* LD T,n8 */
			sect_OutputConst8(0x49);              /* EXT */
			return true;
		} else if (high == low) {
			handle_LD_R8_const(registerLow, low); /* LD r8,n8 */
			sect_OutputConst8(0x78);              /* LD F,T */
			return true;
		}
	}

	return false;
}

static bool
handle_LD_R16_4bytes(SAddressingMode* registerHigh, SAddressingMode* registerLow, uint8_t high, uint8_t low) {
	handle_LD_R8_const(registerHigh, high);
	handle_LD_R8_const(registerLow, low);
	return true;
}

static bool
handle_LD_R16_imm(SAddressingMode* dest, SExpression* expression) {
	SAddressingMode registerLow;
	SAddressingMode registerHigh;
	registerPair(&registerHigh, &registerLow, dest);

	SExpression* masked = expr_CheckRange(expression, -32768, 65535);
	if (!masked)
		return false;

	if (expr_IsConstant(masked)) {
		uint16_t value = masked->value.integer;
		uint8_t high = value >> 8;
		uint8_t low = value & 0xFF;

		if (handle_LD_R16_3bytes(&registerHigh, &registerLow, high, low) ||
		    handle_LD_R16_4bytes(&registerHigh, &registerLow, high, low)) {
			expr_Free(masked);
			return true;
		}
	}

	SExpression* high = expr_Asr(expr_Clone(masked), expr_Const(8));
	SExpression* low = expr_And(masked, expr_Const(0xFF));

	if (high == NULL || low == NULL) {
		expr_Free(high);
		expr_Free(low);
		return false;
	}

	if (!handle_Op_Reg_UnsignedImm(0x80, &registerHigh, high))
		return false;
	return handle_Op_Reg_UnsignedImm(0x80, &registerLow, low);
}

static bool
handle_LD(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	if ((destination->mode & MODE_REG_8BIT) && source->mode == MODE_IMM) {
		bool r = handle_Op_Reg_SignedOrUnsignedImm(0x80, destination, source->expression);
		source->expression = NULL;
		return r;
	} else if ((destination->mode & MODE_REG_16BIT) && source->mode == MODE_IMM &&
	           opt_Current->machineOptions->enableSynthInstructions) {
		bool r = handle_LD_R16_imm(destination, source->expression);
		source->expression = NULL;
		return r;
	} else if ((destination->mode & MODE_IND_16BIT_BCDEHL) && source->mode == MODE_REG_T) {
		return handle_OpcodeRegister(0x00, destination);
	} else if ((destination->mode == MODE_IND_FT) && (source->mode & MODE_REG_8BIT_BCDEHL)) {
		return handle_OpcodeRegister(0x20, source);
	} else if (destination->mode == MODE_REG_T && (source->mode & MODE_IND_16BIT)) {
		return handle_OpcodeRegister(0x04, source);
	} else if ((destination->mode & MODE_REG_8BIT_FBCDEHL) && (source->mode == MODE_IND_FT)) {
		return handle_OpcodeRegister(0x28, destination);
	} else if ((destination->mode & MODE_REG_8BIT_FBCDEHL) && source->mode == MODE_REG_T) {
		return handle_OpcodeRegister(0x78, destination);
	} else if (destination->mode == MODE_REG_T && (source->mode & MODE_REG_8BIT_FBCDEHL)) {
		return handle_OpcodeRegister(0x58, source);
	} else if ((destination->mode & MODE_REG_16BIT_BCDEHL) && source->mode == MODE_REG_FT) {
		return handle_OpcodeRegister(0xD0, destination);
	} else if (destination->mode == MODE_REG_FT && (source->mode & MODE_REG_16BIT_BCDEHL)) {
		return handle_OpcodeRegister(0xD8, source);
	}

	return handle_LongLoad(0x04, 0x00, 0x28, 0x20, cc, true, destination, source);
}

static bool
handle_JAL(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	if (destination->mode == MODE_ADDR && opt_Current->machineOptions->enableSynthInstructions) {
		SAddressingMode mode = {MODE_REG_HL, 3, NULL};
		if (!handle_LD_R16_imm(&mode, destination->expression))
			return false;
		destination->expression = NULL;
		sect_OutputConst8(baseOpcode + 3);
		return true;
	} else if (destination->mode & MODE_IND_16BIT) {
		return handle_OpcodeRegister(baseOpcode, destination);
	}

	return false;
}

static bool
handle_LIO(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	if ((source->mode & MODE_IND_16BIT_INC_DEC) || (destination->mode & MODE_IND_16BIT_INC_DEC) ||
	    (destination->mode == MODE_REG_FT && (source->mode & MODE_IND_16BIT_BCDEHL)) ||
	    ((destination->mode & MODE_IND_16BIT_BCDEHL) && source->mode == MODE_REG_FT))
		return handle_LongLoad(0x34, 0x31, 0xFF, 0xFF, cc, false, destination, source);
	else if ((destination->mode & MODE_IND_16BIT_BCDEHL) && source->mode == MODE_REG_T)
		return handle_OpcodeRegister(baseOpcode, destination);
	else if (destination->mode == MODE_REG_T && (source->mode & MODE_IND_16BIT))
		return handle_OpcodeRegister(baseOpcode + 0x04, source);

	return false;
}

static uint8_t
bitsSet2(uint8_t value) {
	if (value == 0)
		return 0;

	return ((value & 0x2) >> 1) + (value & 1);
}

static uint8_t
bitsSet4(uint8_t value) {
	if (value == 0)
		return 0;

	return bitsSet2(value & 0x03) + bitsSet2(value >> 2);
}

static void
outputStackOps(uint8_t baseOpcode, uint8_t registerMask) {
	while (registerMask != 0) {
		if (registerMask & 1) {
			sect_OutputConst8(baseOpcode);
		}
		registerMask >>= 1;
		baseOpcode += 1;
	}
}

static bool
handle_RegisterStack(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	if (destination->mode == MODE_REGISTER_MASK) {
		if (!opt_Current->machineOptions->enableSynthInstructions)
			return err_Error(MERROR_REQUIRES_SYNTHESIZED);

		bool isPop = baseOpcode == 0xC4;
		bool isSwap = baseOpcode == 0xDC;
		uint8_t all = isSwap ? 0xCC : isPop ? 0xE9 : 0xE8;
		uint8_t oppositeOne = isSwap ? baseOpcode : baseOpcode ^ 0x04;

		uint8_t registerCount = bitsSet4(destination->registerIndex);
		assert(registerCount >= 2 && registerCount <= 4);
		switch (registerCount) {
			case 4:
				sect_OutputConst8(all);
				break;
			case 3:
				if (!isPop)
					sect_OutputConst8(all);

				outputStackOps(oppositeOne, destination->registerIndex ^ 0xF);

				if (isPop)
					sect_OutputConst8(all);
				break;
			case 2:
				outputStackOps(baseOpcode, destination->registerIndex);
				break;
		}
		return true;
	}

	return handle_OpcodeRegister(baseOpcode, destination);
}

static bool
handle_STOW(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	// STOW R16,#i8: native immediate-index stack write. All four pairs
	// are valid, FT included; the index touches no register.
	bool r = handle_Op_Reg_UnsignedImm(baseOpcode, destination, source->expression);
	source->expression = NULL;
	return r;
}

static bool
handle_PICK(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	// PICK R16,#i8: native immediate-index stack read. The synthesized
	// shorthand "PICK FT,#i" (LD T,#i / PICK FT) is now this single
	// instruction, and the removed T-indexed register form needs an
	// explicit LCR rewind to express a variable depth.
	bool r = handle_Op_Reg_UnsignedImm(baseOpcode, destination, source->expression);
	source->expression = NULL;
	return r;
}

static bool
handle_CMP(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	ensureSource(&destination, &source);

	if ((destination->mode & MODE_REG_8BIT) && (source->mode & MODE_IMM)) {
		handle_OpcodeRegister(0xA8, destination);
		sect_OutputExpr8(source->expression);
		source->expression = NULL;
		return true;
	} else if ((destination->mode & (MODE_REG_T | MODE_NONE)) && (source->mode & MODE_REG_8BIT_FBCDEHL)) {
		return handle_OpcodeRegister(0x48, source);
	} else if ((destination->mode & (MODE_REG_FT | MODE_NONE)) && (source->mode & MODE_REG_16BIT_BCDEHL)) {
		return handle_OpcodeRegister(0xCC, source);
	} else if ((destination->mode & MODE_REG_8BIT) && (source->mode & MODE_REG_8BIT)) {
		return handle_SynthRegOp(ALU_CMP, 0x48, 0, cc, destination, source);
	} else if (destination->mode == source->mode && (destination->mode & MODE_REG_16BIT)) {
		// Self-comparison: statically equal.
		if (!opt_Current->machineOptions->enableSynthInstructions)
			return err_Error(MERROR_REQUIRES_SYNTHESIZED);
		return handle_SynthSelfOp(ALU_CMP, cc, destination);
	} else if ((destination->mode & (MODE_REG_16BIT & ~MODE_REG_FT)) && (source->mode & MODE_REG_16BIT)) {
		// CMP R16,R16 through FT:
		//   PUSH X / EXG FT,X / CMP FT,Y / LD T,Xlo / POP X
		// The push saves the destination high byte, which the CMP
		// destroys in F; the hub low byte rides in Xlo across the
		// exchange and the load puts it back before the pop restores the
		// pair. Five instructions, one transient pair-stack slot. When Y
		// is FT, the first exchange parked it in X. The closing step used
		// to be `EXG T,Xlo`; a plain load does the same job now that the
		// byte exchange is out of the ISA, and the pair is popped over
		// that byte a moment later either way.
		if (!opt_Current->machineOptions->enableSynthInstructions)
			return err_Error(MERROR_REQUIRES_SYNTHESIZED);

		SAddressingMode src = *source;
		SAddressingMode high, low;
		registerPair(&high, &low, destination);

		if (src.mode == MODE_REG_FT)
			src = *destination;

		return handle_OpcodeRegister(0xC0, destination) // PUSH X
		       && handle_OpcodeRegister(0xC8, destination) && handle_OpcodeRegister(0xCC, &src) &&
		       handle_OpcodeRegister(0x58, &low) &&       // LD T,Xlo
		       handle_OpcodeRegister(0xC4, destination);  // POP X
	}

	return false;
}

static bool
handle_NOT(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	if (destination->mode & MODE_REG_T) {
		if (!opt_Current->machineOptions->enableSynthInstructions)
			return err_Error(MERROR_REQUIRES_SYNTHESIZED);
		sect_OutputConst8(0xB2); // XOR T,n8
		sect_OutputConst8(0xFF);
		return true;
	} else if (destination->mode & MODE_REG_FT) {
		if (!opt_Current->machineOptions->enableSynthInstructions)
			return err_Error(MERROR_REQUIRES_SYNTHESIZED);
		sect_OutputConst8(0xB2); // XOR T,n8
		sect_OutputConst8(0xFF);
		sect_OutputConst8(baseOpcode);
		return true;
	} else if (destination->mode & MODE_REG_F) {
		sect_OutputConst8(baseOpcode);
		return true;
	}
	/* `not b` and friends would open with the byte exchange through T, which
	 is out of the ISA: spell them as `ld t,b` / `xor t,$ff` / `ld b,t`. */

	return false;
}

static bool
handle_TST(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	return handle_OpcodeRegister(baseOpcode, destination);
}

static bool
handle_NEG(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	if (destination->mode == MODE_REG_T) {
		sect_OutputConst8(0x51); // NEG T
		return true;
	} else if (destination->mode == MODE_REG_FT) {
		sect_OutputConst8(0xF8); // NEG FT
		return true;
	} else if (destination->mode == MODE_REG_F) {
		// Two's complement the flags byte in place: NOT F ($30) then
		// ADD F,i8 ($A0). Two instructions instead of the three the hub
		// exchange used to need, and both forms exist natively for F, so
		// no exchange is involved at all. Same result value (verified over
		// all 256 inputs) and the hub byte is left alone either way.
		if (!opt_Current->machineOptions->enableSynthInstructions)
			return err_Error(MERROR_REQUIRES_SYNTHESIZED);
		sect_OutputConst8(0x30); // NOT F
		handle_OpcodeRegister(0xA0, destination); // ADD F,i8
		sect_OutputConst8(1);
		return true;
	} else if (destination->mode & MODE_REG_16BIT) {
		// exg ft,X / neg ft / exg ft,X
		if (!opt_Current->machineOptions->enableSynthInstructions)
			return err_Error(MERROR_REQUIRES_SYNTHESIZED);
		handle_OpcodeRegister(0xC8, destination);
		sect_OutputConst8(0xF8);
		handle_OpcodeRegister(0xC8, destination);
		return true;
	}
	/* `neg b` opened with the byte exchange through T, which is out of the
	 ISA: spell it `ld t,b` / `neg t` / `ld b,t`. */

	return false;
}

static bool
handle_SYS(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	sect_OutputConst8(baseOpcode);
	sect_OutputExpr8(destination->expression);
	destination->expression = NULL;
	return true;
}

static bool
handle_SUB(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	if (destination->mode == source->mode && (destination->mode & (MODE_REG_8BIT | MODE_REG_16BIT))) {
		// `sub X,X` is zero; `sub t,t` would otherwise encode as NEG T
		// (the SUB T,T opcode slot is taken by NEG T).
		if (!opt_Current->machineOptions->enableSynthInstructions)
			return err_Error(MERROR_REQUIRES_SYNTHESIZED);
		return handle_SynthSelfOp(ALU_SUB, cc, destination);
	}

	ensureSource(&destination, &source);

	if ((source->mode & MODE_REG_8BIT) && (destination->mode & (MODE_NONE | MODE_REG_T)))
		return handle_OpcodeRegister(0x50, source);
	else if ((source->mode & MODE_REG_16BIT_BCDEHL) && (destination->mode & (MODE_NONE | MODE_REG_FT)))
		return handle_OpcodeRegister(0xF8, source);
	else if ((destination->mode & (MODE_REG_8BIT | MODE_REG_16BIT)) && (source->mode & MODE_IMM)) {
		if (!opt_Current->machineOptions->enableSynthInstructions)
			return err_Error(MERROR_REQUIRES_SYNTHESIZED);
		source->expression = expr_Sub(expr_Const(0), source->expression);
		return handle_ADD(0x00, cc, destination, source);
	} else if ((destination->mode & (MODE_REG_8BIT & ~MODE_REG_T)) && (source->mode & MODE_REG_8BIT))
		return handle_SynthRegOp(ALU_SUB, 0x50, 0xF8, cc, destination, source);
	else if ((destination->mode & (MODE_REG_16BIT & ~MODE_REG_FT)) && (source->mode & MODE_REG_16BIT))
		return handle_SynthRegOp(ALU_SUB, 0x50, 0xF8, cc, destination, source);

	return false;
}

static SParser g_Parsers[T_RC8_XOR - T_RC8_ADD + 1] = {
    {0x00, CONDITION_AUTO, MODE_REG_8BIT | MODE_REG_16BIT,                                            MODE_IMM | MODE_REG_8BIT | MODE_REG_16BIT | MODE_NONE,
     handle_ADD                                                                                                                                                                                 }, /* ADD */
    {0x68, CONDITION_AUTO, MODE_REG_8BIT | MODE_IMM,                                                  MODE_NONE | MODE_REG_8BIT | MODE_IMM,                                 handle_Bitwise      }, /* AND */
    {0x00, CONDITION_AUTO, MODE_REG_8BIT | MODE_REG_16BIT,                                            MODE_IMM | MODE_REG_8BIT | MODE_REG_16BIT | MODE_NONE,
     handle_CMP                                                                                                                                                                                 }, /* CMP */
    {0x09, CONDITION_AUTO, MODE_NONE,                                                                 MODE_NONE,                                                            handle_Implicit     }, /* DI */
    {0x88, CONDITION_AUTO, MODE_REG_8BIT,                                                             MODE_ADDR,                                                            handle_DJ           }, /* DJ */
    {0x08, CONDITION_AUTO, MODE_NONE,                                                                 MODE_NONE,                                                            handle_Implicit     }, /* EI */
    {0x10, CONDITION_AUTO, MODE_REG_8BIT | MODE_REG_16BIT,                                            MODE_NONE | MODE_REG_8BIT | MODE_REG_16BIT,                           handle_EXG          }, /* EXG */
    {0x49, CONDITION_AUTO, MODE_NONE,                                                                 MODE_NONE,                                                            handle_Implicit     }, /* EXT */
    {0x3C, CONDITION_PASS, MODE_IND_16BIT | MODE_ADDR,                                                MODE_NONE,                                                            handle_J            }, /* J */
    {0x38, CONDITION_AUTO, MODE_IND_16BIT | MODE_ADDR,                                                MODE_NONE,                                                            handle_JAL          }, /* JAL */
    {0x0C, CONDITION_AUTO, MODE_REG_T | MODE_REG_FT,                                                  MODE_IND_16BIT | MODE_IND_16BIT_INC_DEC,                              handle_LCO          }, /* LCO */
    {0x0A, CONDITION_AUTO, MODE_IND_C | MODE_REG_T,                                                   MODE_IND_C | MODE_REG_T,                                              handle_LCR          }, /* LCR */
    {0x00, CONDITION_AUTO, MODE_REG_8BIT | MODE_REG_16BIT | MODE_IND_16BIT | MODE_IND_16BIT_INC_DEC,
     MODE_IMM | MODE_REG_8BIT | MODE_REG_16BIT | MODE_IND_16BIT | MODE_IND_16BIT_INC_DEC,                                                                                   handle_LD           }, /* LD */
    {0x30, CONDITION_AUTO, MODE_IND_16BIT_INC_DEC | MODE_IND_16BIT_BCDEHL | MODE_REG_T | MODE_REG_FT,
     MODE_IND_16BIT_INC_DEC | MODE_IND_16BIT | MODE_REG_T | MODE_REG_FT,                                                                                                    handle_LIO          }, /* LIO */
    {0xF0, CONDITION_AUTO, MODE_REG_FT | MODE_REG_8BIT_BCDEHL | MODE_IMM,                             MODE_REG_8BIT_BCDEHL | MODE_IMM | MODE_NONE,
     handle_Shift                                                                                                                                                                               }, /* LS */
    {0x51, CONDITION_AUTO, MODE_REG_8BIT | MODE_REG_16BIT,                                            MODE_NONE,                                                            handle_NEG          }, /* NEG */
    {0x00, CONDITION_AUTO, MODE_NONE,                                                                 MODE_NONE,                                                            handle_Implicit     }, /* NOP */
    {0x30, CONDITION_AUTO, MODE_REG_8BIT | MODE_REG_FT,                                               MODE_NONE,                                                            handle_NOT          }, /* NOT */
    {0x60, CONDITION_AUTO, MODE_REG_8BIT | MODE_IMM,                                                  MODE_NONE | MODE_REG_8BIT | MODE_IMM,                                 handle_Bitwise      }, /* OR */
    {0xBC, CONDITION_AUTO, MODE_REG_16BIT,                                                            MODE_IMM,                                                           handle_PICK         }, /* PICK */
    {0xC4, CONDITION_AUTO, MODE_REG_16BIT | MODE_REGISTER_MASK,                                       MODE_NONE,                                                            handle_RegisterStack}, /* POP */
    {0xE9, CONDITION_AUTO, MODE_NONE,                                                                 MODE_NONE,                                                            handle_Implicit     }, /* POPA */
    {0xC0, CONDITION_AUTO, MODE_REG_16BIT | MODE_REGISTER_MASK,                                       MODE_NONE,                                                            handle_RegisterStack}, /* PUSH */
    {0xE8, CONDITION_AUTO, MODE_NONE,                                                                 MODE_NONE,                                                            handle_Implicit     }, /* PUSHA */
    {0x59, CONDITION_AUTO, MODE_NONE,                                                                 MODE_NONE,                                                            handle_Implicit     }, /* RETI */
    {0xE0, CONDITION_AUTO, MODE_REG_FT | MODE_REG_8BIT_BCDEHL | MODE_IMM,                             MODE_REG_8BIT_BCDEHL | MODE_IMM | MODE_NONE,
     handle_Shift                                                                                                                                                                               }, /* RS */
    {0xE8, CONDITION_AUTO, MODE_REG_FT | MODE_REG_8BIT_BCDEHL | MODE_IMM,                             MODE_REG_8BIT_BCDEHL | MODE_IMM | MODE_NONE,
     handle_Shift                                                                                                                                                                               }, /* RSA */
    {0xB4, CONDITION_AUTO, MODE_REG_16BIT,                                                            MODE_IMM,                                                           handle_STOW         }, /* STOW */
    {0x00, CONDITION_AUTO, MODE_REG_8BIT | MODE_REG_16BIT,                                            MODE_NONE | MODE_REG_8BIT | MODE_REG_16BIT | MODE_IMM,
     handle_SUB                                                                                                                                                                                 }, /* SUB */
    {0xDC, CONDITION_AUTO, MODE_REG_16BIT | MODE_REGISTER_MASK,                                       MODE_NONE,                                                            handle_RegisterStack}, /* SWAP */
    {0xCC, CONDITION_AUTO, MODE_NONE,                                                                 MODE_NONE,                                                            handle_Implicit     }, /* SWAPA */
    {0x9B, CONDITION_AUTO, MODE_IMM,                                                                  MODE_NONE,                                                            handle_SYS          }, /* SYS */
    {0xD4, CONDITION_AUTO, MODE_REG_16BIT,                                                            MODE_NONE,                                                            handle_TST          }, /* TST */
    {0x70, CONDITION_AUTO, MODE_REG_8BIT | MODE_IMM,                                                  MODE_NONE | MODE_REG_8BIT | MODE_IMM,                                 handle_Bitwise      }, /* XOR */
};

typedef struct RegisterMode {
	uint32_t modeFlag;
	uint16_t token;
	uint8_t opcodeRegister;
} SRegisterMode;

static SRegisterMode g_RegisterModes[T_RC8_REG_HL_IND_PRE_DEC - T_RC8_REG_F + 1] = {
    {MODE_REG_F,               T_RC8_REG_F,               0},
    {MODE_REG_T,               T_RC8_REG_T,               1},
    {MODE_REG_B,               T_RC8_REG_B,               2},
    {MODE_REG_C,               T_RC8_REG_C,               3},
    {MODE_REG_D,               T_RC8_REG_D,               4},
    {MODE_REG_E,               T_RC8_REG_E,               5},
    {MODE_REG_H,               T_RC8_REG_H,               6},
    {MODE_REG_L,               T_RC8_REG_L,               7},
    {MODE_REG_FT,              T_RC8_REG_FT,              0},
    {MODE_REG_BC,              T_RC8_REG_BC,              1},
    {MODE_REG_DE,              T_RC8_REG_DE,              2},
    {MODE_REG_HL,              T_RC8_REG_HL,              3},
    {MODE_IND_C,               T_RC8_REG_C_IND,           0},
    {MODE_IND_FT,              T_RC8_REG_FT_IND,          0},
    {MODE_IND_BC,              T_RC8_REG_BC_IND,          1},
    {MODE_IND_DE,              T_RC8_REG_DE_IND,          2},
    {MODE_IND_HL,              T_RC8_REG_HL_IND,          3},
    {MODE_IND_FT_POST_INC,     T_RC8_REG_FT_IND_POST_INC, 0},
    {MODE_IND_BCDEHL_POST_INC, T_RC8_REG_BC_IND_POST_INC, 1},
    {MODE_IND_BCDEHL_POST_INC, T_RC8_REG_DE_IND_POST_INC, 2},
    {MODE_IND_BCDEHL_POST_INC, T_RC8_REG_HL_IND_POST_INC, 3},
    {MODE_IND_FT_PRE_DEC,      T_RC8_REG_FT_IND_PRE_DEC,  0},
    {MODE_IND_BCDEHL_PRE_DEC,  T_RC8_REG_BC_IND_PRE_DEC,  1},
    {MODE_IND_BCDEHL_PRE_DEC,  T_RC8_REG_DE_IND_PRE_DEC,  2},
    {MODE_IND_BCDEHL_PRE_DEC,  T_RC8_REG_HL_IND_PRE_DEC,  3},
};

static bool
parseAddressingMode(SAddressingMode* addrMode, int allowedModes) {
	SLexerContext bm;
	lex_Bookmark(&bm);

	if (allowedModes & MODE_REGISTER_MASK) {
		uint8_t mask = 0;
		uint8_t registers = 0;

		while (lex_Context->token.id >= T_RC8_REG_FT && lex_Context->token.id <= T_RC8_REG_HL) {
			mask |= 1 << (lex_Context->token.id - T_RC8_REG_FT);
			registers += 1;

			parse_GetToken();

			if (lex_Context->token.id == T_OP_DIVIDE) {
				parse_GetToken();
				continue;
			} else {
				break;
			}
		}

		if (registers >= 2) {
			addrMode->mode = MODE_REGISTER_MASK;
			addrMode->expression = NULL;
			addrMode->registerIndex = mask;

			return true;
		}

		lex_Goto(&bm);
	}

	if (lex_Context->token.id >= T_RC8_REG_F && lex_Context->token.id <= T_RC8_REG_HL_IND_PRE_DEC) {
		SRegisterMode* mode = &g_RegisterModes[lex_Context->token.id - T_RC8_REG_F];
		if (allowedModes & mode->modeFlag) {
			parse_GetToken();
			addrMode->mode = mode->modeFlag;
			addrMode->registerIndex = mode->opcodeRegister;
			return true;
		}
	}

	if (lex_Context->token.id == '(' && (allowedModes & MODE_IND_16BIT)) {
		parse_GetToken();
		if (lex_Context->token.id >= T_RC8_REG_FT && lex_Context->token.id <= T_RC8_REG_HL) {
			int ind_token = lex_Context->token.id - T_RC8_REG_FT + T_RC8_REG_FT_IND;
			parse_GetToken();
			if (lex_Context->token.id == ')') {
				parse_GetToken();
				SRegisterMode* mode = &g_RegisterModes[ind_token - T_RC8_REG_F];
				if (allowedModes & mode->modeFlag) {
					addrMode->mode = mode->modeFlag;
					addrMode->registerIndex = mode->opcodeRegister;
					return true;
				}
			}
		}
		lex_Goto(&bm);
	}

	if (allowedModes & MODE_ADDR) {
		addrMode->mode = MODE_ADDR;
		addrMode->expression = parse_Expression(1);

		if (addrMode->expression != NULL)
			return true;

		lex_Goto(&bm);
	}

	if (allowedModes & MODE_IMM) {
		addrMode->mode = MODE_IMM;
		addrMode->expression = rc8_ExpressionSU16();

		if (addrMode->expression != NULL)
			return true;

		lex_Goto(&bm);
	}

	if ((allowedModes == 0) || (allowedModes & MODE_NONE)) {
		addrMode->mode = MODE_NONE;
		addrMode->expression = NULL;
		return true;
	}

	return false;
}

static bool
rc8_ParseInstructionBody(SParser* parser, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	EConditionCode cc = CC_ALWAYS;

	parse_GetToken();

	if (lex_Context->token.id == T_OP_DIVIDE) {
		parse_GetToken();

		if (lex_Context->token.id == T_RC8_REG_C) {
			cc = CC_LTU;
		} else if (lex_Context->token.id >= T_RC8_CC_LE && lex_Context->token.id <= T_RC8_CC_NE) {
			cc = lex_Context->token.id - T_RC8_CC_LE;
		} else {
			err_Error(MERROR_EXPECTED_CONDITION_CODE);
			return false;
		}
		parse_GetToken();
	}

	if (!parseAddressingMode(addrMode1, parser->firstModes)) {
		err_Error(ERROR_OPERAND);
		return false;
	}

	string* jumpTarget = NULL;

	if (lex_Context->token.id == ',') {
		parse_GetToken();
		if (!parseAddressingMode(addrMode2, parser->secondModes)) {
			err_Error(MERROR_ILLEGAL_ADDRMODE);
			return false;
		}
	} else if ((parser->secondModes & MODE_NONE) || (parser->secondModes == 0)) {
		addrMode2->mode = MODE_NONE;
		addrMode2->expression = NULL;
	} else {
		err_Error(MERROR_ILLEGAL_ADDRMODE);
		return false;
	}

	if (parser->condition == CONDITION_AUTO && cc != CC_ALWAYS) {
		jumpTarget = createUniqueLabel();
		handle_JumpRelativeCC(invertCondition(cc), expr_SymbolByName(jumpTarget));
	}

	if (!parser->parser(parser->baseOpcode, cc, addrMode1, addrMode2)) {
		err_Error(ERROR_OPERAND);
		return false;
	}

	if (jumpTarget != NULL) {
		sym_CreateLabel(jumpTarget);
		str_Free(jumpTarget);
	}

	return true;
}

bool
rc8_ParseIntegerInstruction(void) {
	if (T_RC8_ADD <= lex_Context->token.id && lex_Context->token.id <= T_RC8_XOR) {
		SAddressingMode addrMode1 = {0};
		SAddressingMode addrMode2 = {0};
		SParser* parser = &g_Parsers[lex_Context->token.id - T_RC8_ADD];

		bool result = rc8_ParseInstructionBody(parser, &addrMode1, &addrMode2);

		addrMode_FreeAll(&addrMode1);
		addrMode_FreeAll(&addrMode2);
		return result;
	}

	return false;
}
