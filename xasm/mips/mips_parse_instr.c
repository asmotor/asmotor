/*  Copyright 2008-2017 Carsten Elton Sorensen

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

#include "errors.h"
#include "lexer.h"
#include "parse.h"
#include "parse_expression.h"
#include "section.h"

#include "mips_errors.h"
#include "mips_tokens.h"

static int
parse_GetRegister(void) {
    if (lex_Current.token >= T_MIPS_REG_R0 && lex_Current.token <= T_MIPS_REG_R31) {
        int r = lex_Current.token - T_MIPS_REG_R0;
        parse_GetToken();

        return r;
    }

    return -1;
}

static int
parse_ExpectRegister(void) {
    int result = parse_GetRegister();

    if (result == -1)
        err_Error(MERROR_REGISTER_EXPECTED);

    return result;
}

static uint32_t s_InstructionsRRR[T_MIPS_INTEGER_RRR_LAST - T_MIPS_INTEGER_RRR_FIRST + 1] = {
    0x00000020, // ADD
    0x00000021, // ADDU
    0x00000024, // AND
    0x0000000B, // MOVN
    0x0000000A, // MOVZ,
    0x70000002, // MUL
    0x00000027, // NOR
    0x00000025, // OR
    0x00000046, // ROTRV
    0x00000004, // SLLV
    0x0000002A, // SLT
    0x0000002B, // SLTU
    0x00000007, // SRAV
    0x00000006, // SRLV
    0x00000022, // SUB
    0x00000023, // SUBU
    0x00000026, // XOR
};

static bool
parse_IntegerInstructionRRR(void) {
    if (lex_Current.token >= T_MIPS_INTEGER_RRR_FIRST && lex_Current.token <= T_MIPS_INTEGER_RRR_LAST) {
        uint32_t opcode = s_InstructionsRRR[lex_Current.token - T_MIPS_INTEGER_RRR_FIRST];

        parse_GetToken();

        int rd = parse_ExpectRegister();
        if (rd == -1)
            return false;

        if (!parse_ExpectChar(','))
            return false;

        int rs = parse_ExpectRegister();
        if (rs == -1)
            return false;

        if (!parse_ExpectChar(','))
            return false;

        int rt = parse_ExpectRegister();
        if (rt == -1)
            return false;

        sect_OutputConst32(opcode | (rs << 21) | (rt << 16) | (rd << 11));
        return true;
    }

    return false;
}

typedef struct {
    uint32_t opcode;
    bool isSigned;
} SInstructionRRI;

static SInstructionRRI g_RRIInstructions[T_MIPS_INTEGER_RRI_LAST - T_MIPS_INTEGER_RRI_FIRST + 1] = {
    {0x20000000, true},     // ADDI
    {0x24000000, true},     // ADDIU
    {0x30000000, false},    // ANDI
    {0x34000000, false},    // ORI
    {0x28000000, true},     // SLTI
    {0x2C000000, false},    // SLTIU
    {0x38000000, false},    // XORI
};

static bool
parse_IntegerInstructionRRI(void) {
    if (lex_Current.token >= T_MIPS_INTEGER_RRI_FIRST && lex_Current.token <= T_MIPS_INTEGER_RRI_LAST) {
        SInstructionRRI* instruction = &g_RRIInstructions[lex_Current.token - T_MIPS_INTEGER_RRI_FIRST];

        parse_GetToken();

        int rt = parse_ExpectRegister();
        if (rt == -1)
            return false;

        if (!parse_ExpectChar(','))
            return false;

        int rs = parse_ExpectRegister();
        if (rs == -1)
            return false;

        if (!parse_ExpectChar(','))
            return false;

        SExpression* expression;
        if (instruction->isSigned)
            expression = parse_ExpressionS16();
        else
            expression = parse_ExpressionU16();

        if (expression != NULL) {
            expression = expr_Or(expression, expr_Const((rs << 21) | (rt << 16) | instruction->opcode));

            sect_OutputExpr32(expression);
        }
        return true;
    }

    return false;
}

typedef struct {
    int registerCount;
    uint32_t opcode;
} SInstructionBranch;

static SInstructionBranch g_branchInstructions[T_MIPS_BRANCH_LAST - T_MIPS_BRANCH_FIRST + 1] = {
    {2, 0x10000000}, // BEQ
    {2, 0x50000000}, // BEQL
    {2, 0x14000000}, // BNE
    {2, 0x54000000}, // BNEL

    /* Branch R,addr */

    {1, 0x04010000}, // BGEZ
    {1, 0x04110000}, // BGEZAL
    {1, 0x04130000}, // BGEZALL
    {1, 0x04030000}, // BGEZL
    {1, 0x1C000000}, // BGTZ
    {1, 0x5C000000}, // BGTZL
    {1, 0x18000000}, // BLEZ
    {1, 0x58000000}, // BLEZL
    {1, 0x04000000}, // BLTZ
    {1, 0x04100000}, // BLTZAL
    {1, 0x04120000}, // BLTZALL
    {1, 0x02020000}, // BLTZL

    /* Branch addr */

    {0, 0x10000000}, // B
    {0, 0x04110000}, // BAL
};

static bool
parse_Branch(void) {
    if (lex_Current.token >= T_MIPS_BRANCH_FIRST && lex_Current.token <= T_MIPS_BRANCH_LAST) {
        SInstructionBranch* instruction = &g_branchInstructions[lex_Current.token - T_MIPS_BRANCH_FIRST];

        parse_GetToken();

        int rs = 0;
        if (instruction->registerCount >= 1) {
            rs = parse_ExpectRegister();
            if (rs == -1)
                return false;

            if (!parse_ExpectChar(','))
                return false;
        }

        int rt = 0;
        if (instruction->registerCount >= 2) {
            rt = parse_ExpectRegister();
            if (rt == -1)
                return false;

            if (!parse_ExpectChar(','))
                return false;
        }

        SExpression* expression = parse_Expression(4);
        if (expression == NULL)
            return false;

        expression = expr_PcRelative(expression, -4);
        expression = expr_Asr(expression, expr_Const(2));
        expression = expr_CheckRange(expression, -32768, 32767);
        if (expression != NULL) {
            expression = expr_And(expression, expr_Const(0xFFFF));

            expression = expr_Or(expression, expr_Const(instruction->opcode | (rs << 21) | (rt << 16)));

            sect_OutputExpr32(expression);
        } else {
            err_Error(ERROR_OPERAND_RANGE);
        }

        return true;
    }

    return false;
}

static uint32_t g_shiftInstructions[T_MIPS_SHIFT_LAST - T_MIPS_SHIFT_FIRST + 1] = {
    0x00200002, // ROTR
    0x00000000, // SLL
    0x00000003, // SRA
    0x00000002, // SRL
};

static bool
parse_Shift(void) {
    if (lex_Current.token >= T_MIPS_SHIFT_FIRST && lex_Current.token <= T_MIPS_SHIFT_LAST) {
        uint32_t opcode = g_shiftInstructions[lex_Current.token - T_MIPS_SHIFT_FIRST];

        parse_GetToken();

        int rd = parse_ExpectRegister();
        if (rd == -1)
            return false;

        if (!parse_ExpectChar(','))
            return false;

        int rt = parse_ExpectRegister();
        if (rt == -1)
            return false;

        if (!parse_ExpectChar(','))
            return false;

        SExpression* expression = parse_Expression(1);
        if (expression == NULL)
            return false;

        expression = expr_CheckRange(expression, 0, 31);
        if (expression != NULL) {
            expression = expr_Asl(expression, expr_Const(6));
            expression = expr_Or(expression, expr_Const(opcode | (rt << 16) | (rd << 11)));

            sect_OutputExpr32(expression);
        } else {
            err_Error(ERROR_OPERAND_RANGE);
        }

        return true;
    }

    return false;
}

static uint32_t g_loadStoreInstructions[T_MIPS_LOADSTORE_LAST - T_MIPS_LOADSTORE_FIRST + 1] = {
    0x80000000,    // LB
    0x90000000,    // LBU
    0x84000000,    // LH
    0x94000000,    // LHU
    0xC0000000,    // LL
    0x8C000000,    // LW
    0xC4000000,    // LWC1
    0xC8000000,    // LWC2
    0x88000000,    // LWL
    0x98000000,    // LWR
    0xA0000000,    // SB
    0xE0000000,    // SC
    0xA4000000,    // SH
    0xAC000000,    // SW
    0xE4000000,    // SWC1
    0xE8000000,    // SWC2
    0xA8000000,    // SWL
    0xB8000000,    // SWR
};

static bool
parse_LoadStore(void) {
    if (lex_Current.token >= T_MIPS_LOADSTORE_FIRST && lex_Current.token <= T_MIPS_LOADSTORE_LAST) {
        uint32_t opcode = g_loadStoreInstructions[lex_Current.token - T_MIPS_LOADSTORE_FIRST];

        parse_GetToken();

        int rt = parse_ExpectRegister();
        if (rt == -1)
            return false;

        if (!parse_ExpectChar(','))
            return false;

        SExpression* expression = parse_ExpressionS16();
        if (expression == NULL)
            return false;

        if (!parse_ExpectChar('('))
            return false;

        int base = parse_ExpectRegister();
        if (base == -1)
            return false;

        if (parse_ExpectChar(')')) {
            expression = expr_Or(expression, expr_Const(opcode | (base << 21) | (rt << 16)));
            sect_OutputExpr32(expression);
            return true;
        }
    }

    return false;
}

static uint32_t g_RSRTInstructions[T_MIPS_RSRT_LAST - T_MIPS_RSRT_FIRST + 1] = {
    0x0000001A,    // DIV
    0x0000001B,    // DIVU
    0x70000000,    // MADD
    0x70000001,    // MADDU
    0x70000004,    // MSUB
    0x70000005,    // MSUBU
    0x00000018,    // MULT
    0x00000019,    // MULU
};

static bool
parse_IntegerInstructionRSRT(void) {
    if (lex_Current.token >= T_MIPS_RSRT_FIRST && lex_Current.token <= T_MIPS_RSRT_LAST) {
        uint32_t opcode = g_RSRTInstructions[lex_Current.token - T_MIPS_RSRT_FIRST];

        parse_GetToken();

        int rs = parse_ExpectRegister();
        if (rs == -1)
            return false;

        if (!parse_ExpectChar(','))
            return false;

        int rt = parse_ExpectRegister();
        if (rt != -1) {
            sect_OutputConst32(opcode | (rs << 21) | (rt << 16));
            return true;
        }
    }

    return false;
}

static uint32_t g_RDRTInstructions[T_MIPS_RDRT_LAST - T_MIPS_RDRT_FIRST + 1] = {
    0x41400000,    // RDPGPR
    0x7C000420,    // SEB
    0x7C000620,    // SEH
    0x41C00000,    // WRPGPR
    0x7C0000A0,    // WSBH
};

static bool
parse_IntegerInstructionRDRT(void) {
    if (lex_Current.token >= T_MIPS_RDRT_FIRST && lex_Current.token <= T_MIPS_RDRT_LAST) {
        uint32_t opcode = g_RDRTInstructions[lex_Current.token - T_MIPS_RDRT_FIRST];

        parse_GetToken();

        int rd = parse_ExpectRegister();
        if (rd == -1)
            return false;

        if (!parse_ExpectChar(','))
            return false;

        int rt = parse_ExpectRegister();
        if (rt != -1) {
            sect_OutputConst32(opcode | (rd << 11) | (rt << 16));
            return true;
        }
    }

    return false;
}

static uint32_t g_RSRTCodeInstructions[T_MIPS_RSRTCODE_LAST - T_MIPS_RSRTCODE_FIRST + 1] = {
    0x00000034,    // TEQ
    0x00000030,    // TGE
    0x00000031,    // TGEU
    0x00000032,    // TLT
    0x00000033,    // TLTU
    0x00000036,    // TNE
};

static bool
parse_IntegerInstructionRSRTCode(void) {
    if (lex_Current.token >= T_MIPS_RSRTCODE_FIRST && lex_Current.token <= T_MIPS_RSRTCODE_LAST) {
        uint32_t opcode = g_RSRTCodeInstructions[lex_Current.token - T_MIPS_RSRTCODE_FIRST];

        parse_GetToken();

        int rs = parse_ExpectRegister();
        if (rs == -1)
            return false;

        if (!parse_ExpectChar(','))
            return false;

        int rt = parse_ExpectRegister();
        if (rt != -1) {
            if (lex_Current.token == ',') {
                parse_GetToken();

                SExpression* expression = parse_Expression(2);
                if (expression == NULL) {
                    err_Error(ERROR_INVALID_EXPRESSION);
                    return true;
                }

                expression = expr_CheckRange(expression, 0, 1023);
                if (expression == NULL) {
                    err_Error(ERROR_OPERAND_RANGE);
                    return true;
                }

                expression = expr_Asl(expression, expr_Const(6));
                expression = expr_Or(expression, expr_Const(opcode | (rs << 21) | (rt << 16)));
                sect_OutputExpr32(expression);
            } else {
                sect_OutputConst32(opcode | (rs << 21) | (rt << 16));
            }

            return true;
        }
    }

    return false;
}

static SInstructionRRI g_RIInstructions[T_MIPS_INTEGER_RI_LAST - T_MIPS_INTEGER_RI_FIRST + 1] = {
    {0x040C0000, true},     // TEQI
    {0x04080000, true},     // TGEI
    {0x04090000, false},    // TGEIU
    {0x040A0000, true},     // TLTI
    {0x040B0000, false},    // TLTIU
    {0x040E0000, true},     // TNEI
};

static bool
parse_IntegerInstructionRI(void) {
    if (lex_Current.token >= T_MIPS_INTEGER_RI_FIRST && lex_Current.token <= T_MIPS_INTEGER_RI_LAST) {
        SInstructionRRI* instruction = &g_RIInstructions[lex_Current.token - T_MIPS_INTEGER_RI_FIRST];

        parse_GetToken();
        int rs = parse_ExpectRegister();
        if (rs == -1)
            return false;

        if (!parse_ExpectChar(','))
            return false;

        SExpression* expression = instruction->isSigned ? parse_ExpressionS16() : parse_ExpressionU16();

        if (expression != NULL) {
            expression = expr_Or(expression, expr_Const(instruction->opcode | (rs << 21)));
            sect_OutputExpr32(expression);
            return true;
        }
    }

    return false;
}

static uint32_t g_RDRSRTCopyInstructions[T_MIPS_INTEGER_RDRS_RTCOPY_LAST - T_MIPS_INTEGER_RDRS_RTCOPY_FIRST + 1] = {
    0x70000021,    // CLO
    0x70000020    // CLZ
};

static bool
parse_IntegerInstructionRDRSRTCopy(void) {
    if (lex_Current.token >= T_MIPS_INTEGER_RDRS_RTCOPY_FIRST && lex_Current.token <= T_MIPS_INTEGER_RDRS_RTCOPY_LAST) {
        uint32_t opcode = g_RDRSRTCopyInstructions[lex_Current.token - T_MIPS_INTEGER_RDRS_RTCOPY_FIRST];

        parse_GetToken();

        int rd = parse_ExpectRegister();
        if (rd == -1)
            return false;

        if (!parse_ExpectChar(','))
            return false;

        int rs = parse_ExpectRegister();
        if (rs != -1) {
            sect_OutputConst32(opcode | (rd << 11) | (rs << 21) | (rd << 16));

            return true;
        }
    }

    return false;
}

static uint32_t g_noParameterInstructions[T_MIPS_INTEGER_NO_PARAMETER_LAST - T_MIPS_INTEGER_NO_PARAMETER_FIRST + 1] = {
    0x4200001F,    // DERET
    0x000000C0,    // EHB
    0x42000018,    // ERET
    0x00000000,    // NOP
    0x00000040,    // SSNOP
    0x42000008,    // TLBP
    0x42000001,    // TLBR
    0x42000002,    // TLBWI
    0x42000006,    // TLBWR
};

static bool
parse_IntegerNoParameter(void) {
    if (lex_Current.token >= T_MIPS_INTEGER_NO_PARAMETER_FIRST && lex_Current.token <= T_MIPS_INTEGER_NO_PARAMETER_LAST) {
        uint32_t opcode = g_noParameterInstructions[lex_Current.token - T_MIPS_INTEGER_NO_PARAMETER_FIRST];

        parse_GetToken();

        sect_OutputConst32(opcode);
        return true;
    }

    return false;
}

static uint32_t g_RTInstructions[T_MIPS_INTEGER_RT_LAST - T_MIPS_INTEGER_RT_FIRST + 1] = {
    0x41606000,    // DI
    0x41606020,    // EI
};

static bool
parse_IntegerRT(void) {
    if (lex_Current.token >= T_MIPS_INTEGER_RT_FIRST && lex_Current.token <= T_MIPS_INTEGER_RT_LAST) {
        uint32_t opcode = g_RTInstructions[lex_Current.token - T_MIPS_INTEGER_RT_FIRST];

        parse_GetToken();

        int rt = parse_GetRegister();
        if (rt == -1)
            rt = 0;

        sect_OutputConst32(opcode | (rt << 16));

        return true;
    }

    return false;
}

static uint32_t g_RDInstructions[T_MIPS_INTEGER_RD_LAST - T_MIPS_INTEGER_RD_FIRST + 1] = {
    0x00000010,    // MFHI
    0x00000012,    // MFLO
};

static bool
parse_IntegerRD(void) {
    if (lex_Current.token >= T_MIPS_INTEGER_RD_FIRST && lex_Current.token <= T_MIPS_INTEGER_RD_LAST) {
        uint32_t opcode = g_RDInstructions[lex_Current.token - T_MIPS_INTEGER_RD_FIRST];

        parse_GetToken();

        int rd = parse_ExpectRegister();
        if (rd == -1)
            return false;

        sect_OutputConst32(opcode | (rd << 11));
        return true;
    }

    return false;
}

static uint32_t g_RSInstructions[T_MIPS_INTEGER_RS_LAST - T_MIPS_INTEGER_RS_FIRST + 1] = {
    0x00000008,    // JR
    0x00000408,    // JR.HB
    0x00000011,    // MTHI
    0x00000013,    // MTLO
};

static bool
parse_IntegerRS(void) {
    if (lex_Current.token >= T_MIPS_INTEGER_RS_FIRST && lex_Current.token <= T_MIPS_INTEGER_RS_LAST) {
        uint32_t opcode = g_RSInstructions[lex_Current.token - T_MIPS_INTEGER_RS_FIRST];

        parse_GetToken();

        int rs = parse_ExpectRegister();
        if (rs == -1)
            return false;

        sect_OutputConst32(opcode | (rs << 21));
        return true;
    }

    return false;
}

static uint32_t g_jumpAbsInstructions[T_MIPS_INTEGER_J_ABS_LAST - T_MIPS_INTEGER_J_ABS_FIRST + 1] = {
    0x08000000,    // J
    0x0C000000,    // JAL
};

static bool
parse_IntegerJumpAbs(void) {
    if (lex_Current.token >= T_MIPS_INTEGER_J_ABS_FIRST && lex_Current.token <= T_MIPS_INTEGER_J_ABS_LAST) {
        uint32_t opcode = g_jumpAbsInstructions[lex_Current.token - T_MIPS_INTEGER_J_ABS_FIRST];

        parse_GetToken();

        SExpression* expression = parse_Expression(4);
        if (expression != NULL) {
            expression = expr_Asr(expression, expr_Const(2));
            expression = expr_And(expression, expr_Const(0x03FFFFFF));
            expression = expr_Or(expression, expr_Const(opcode));

            sect_OutputExpr32(expression);
            return true;
        }
    }
    return false;
}

static bool
parse_LUI(void) {
    if (lex_Current.token == T_MIPS_LUI) {
        parse_GetToken();

        int rd = parse_ExpectRegister();
        if (rd != -1) {
            if (parse_ExpectChar(',')) {
                SExpression* expression = parse_ExpressionU16();
                if (expression != NULL) {
                    expression = expr_Or(expression, expr_Const((15 << 26) | (rd << 16)));
                    sect_OutputExpr32(expression);
                }
            }
        }
        return true;
    }

    return false;
}

typedef bool (* mnemonicHandler)(void);

mnemonicHandler s_mnemonicHandlers[T_MIPS_LUI - T_MIPS_ADD + 1] = {
    parse_IntegerInstructionRRR,    //	T_MIPS_ADD = 6000,
    parse_IntegerInstructionRRR,    //	T_MIPS_ADDU,
    parse_IntegerInstructionRRR,    //	T_MIPS_AND,
    parse_IntegerInstructionRRR,    //	T_MIPS_MOVN,
    parse_IntegerInstructionRRR,    //	T_MIPS_MOVZ,
    parse_IntegerInstructionRRR,    //	T_MIPS_MUL,
    parse_IntegerInstructionRRR,    //	T_MIPS_NOR,
    parse_IntegerInstructionRRR,    //	T_MIPS_OR,
    parse_IntegerInstructionRRR,    //	T_MIPS_ROTRV,
    parse_IntegerInstructionRRR,    //	T_MIPS_SLLV,
    parse_IntegerInstructionRRR,    //	T_MIPS_SLT,
    parse_IntegerInstructionRRR,    //	T_MIPS_SLTU,
    parse_IntegerInstructionRRR,    //	T_MIPS_SRAV,
    parse_IntegerInstructionRRR,    //	T_MIPS_SRLV,
    parse_IntegerInstructionRRR,    //	T_MIPS_SUB,
    parse_IntegerInstructionRRR,    //	T_MIPS_SUBU,
    parse_IntegerInstructionRRR,    //	T_MIPS_XOR,

    parse_IntegerInstructionRRI,    //	T_MIPS_ADDI,
    parse_IntegerInstructionRRI,    //	T_MIPS_ADDIU,
    parse_IntegerInstructionRRI,    //	T_MIPS_ANDI,
    parse_IntegerInstructionRRI,    //	T_MIPS_ORI,
    parse_IntegerInstructionRRI,    //	T_MIPS_SLTI,
    parse_IntegerInstructionRRI,    //	T_MIPS_SLTIU,
    parse_IntegerInstructionRRI,    //	T_MIPS_XORI,

    parse_Branch,    //	T_MIPS_BEQ,
    parse_Branch,    //	T_MIPS_BEQL,
    parse_Branch,    //	T_MIPS_BNE,
    parse_Branch,    //	T_MIPS_BNEL,
    parse_Branch,    //	T_MIPS_BGEZ,
    parse_Branch,    //	T_MIPS_BGEZAL,
    parse_Branch,    //	T_MIPS_BGEZALL,
    parse_Branch,    //	T_MIPS_BGEZL,
    parse_Branch,    //	T_MIPS_BGTZ,
    parse_Branch,    //	T_MIPS_BGTZL,
    parse_Branch,    //	T_MIPS_BLEZ,
    parse_Branch,    //	T_MIPS_BLEZL,
    parse_Branch,    //	T_MIPS_BLTZ,
    parse_Branch,    //	T_MIPS_BLTZAL,
    parse_Branch,    //	T_MIPS_BLTZALL,
    parse_Branch,    //	T_MIPS_BLTZL,
    parse_Branch,    //	T_MIPS_B,
    parse_Branch,    //	T_MIPS_BAL,

    parse_Shift,    //	T_MIPS_ROTR,
    parse_Shift,    //	T_MIPS_SLL,
    parse_Shift,    //	T_MIPS_SRA,
    parse_Shift,    //	T_MIPS_SRL,

    parse_LoadStore,    //	T_MIPS_LB,
    parse_LoadStore,    //	T_MIPS_LBU,
    parse_LoadStore,    //	T_MIPS_LH,
    parse_LoadStore,    //	T_MIPS_LHU,
    parse_LoadStore,    //	T_MIPS_LL,
    parse_LoadStore,    //	T_MIPS_LW,
    parse_LoadStore,    //	T_MIPS_LWC1,
    parse_LoadStore,    //	T_MIPS_LWC2,
    parse_LoadStore,    //	T_MIPS_LWL,
    parse_LoadStore,    //	T_MIPS_LWR,
    parse_LoadStore,    //	T_MIPS_SB,
    parse_LoadStore,    //	T_MIPS_SC,
    parse_LoadStore,    //	T_MIPS_SH,
    parse_LoadStore,    //	T_MIPS_SW,
    parse_LoadStore,    //	T_MIPS_SWC1,
    parse_LoadStore,    //	T_MIPS_SWC2,
    parse_LoadStore,    //	T_MIPS_SWL,
    parse_LoadStore,    //	T_MIPS_SWR,

    parse_IntegerInstructionRSRT,    //	T_MIPS_DIV,
    parse_IntegerInstructionRSRT,    //	T_MIPS_DIVU,
    parse_IntegerInstructionRSRT,    //	T_MIPS_MADD,
    parse_IntegerInstructionRSRT,    //	T_MIPS_MADDU,
    parse_IntegerInstructionRSRT,    //	T_MIPS_MSUB,
    parse_IntegerInstructionRSRT,    //	T_MIPS_MSUBU,
    parse_IntegerInstructionRSRT,    //	T_MIPS_MULT,
    parse_IntegerInstructionRSRT,    //	T_MIPS_MULU,

    parse_IntegerInstructionRDRT,    //	T_MIPS_RDPGPR,
    parse_IntegerInstructionRDRT,    //	T_MIPS_SEB,
    parse_IntegerInstructionRDRT,    //	T_MIPS_SEH,
    parse_IntegerInstructionRDRT,    //	T_MIPS_WRPGPR,
    parse_IntegerInstructionRDRT,    //	T_MIPS_WSBH,

    parse_IntegerInstructionRSRTCode,    //	T_MIPS_TEQ,
    parse_IntegerInstructionRSRTCode,    //	T_MIPS_TGE,
    parse_IntegerInstructionRSRTCode,    //	T_MIPS_TGEU,
    parse_IntegerInstructionRSRTCode,    //	T_MIPS_TLT,
    parse_IntegerInstructionRSRTCode,    //	T_MIPS_TLTU,
    parse_IntegerInstructionRSRTCode,    //	T_MIPS_TNE,

    parse_IntegerInstructionRI,    //	T_MIPS_TEQI,
    parse_IntegerInstructionRI,    //	T_MIPS_TGEI,
    parse_IntegerInstructionRI,    //	T_MIPS_TGEIU,
    parse_IntegerInstructionRI,    //	T_MIPS_TLTI,
    parse_IntegerInstructionRI,    //	T_MIPS_TLTIU,
    parse_IntegerInstructionRI,    //	T_MIPS_TNEI,

    parse_IntegerInstructionRDRSRTCopy,    //	T_MIPS_CLO,
    parse_IntegerInstructionRDRSRTCopy,    //	T_MIPS_CLZ,

    parse_IntegerNoParameter,    //	T_MIPS_DERET,
    parse_IntegerNoParameter,    //	T_MIPS_EHB,
    parse_IntegerNoParameter,    //	T_MIPS_ERET,
    parse_IntegerNoParameter,    //	T_MIPS_NOP,
    parse_IntegerNoParameter,    //	T_MIPS_SSNOP,
    parse_IntegerNoParameter,    //	T_MIPS_TLBP,
    parse_IntegerNoParameter,    //	T_MIPS_TLBR,
    parse_IntegerNoParameter,    //	T_MIPS_TLBWI,
    parse_IntegerNoParameter,    //	T_MIPS_TLBWR,

    parse_IntegerRT,    //	T_MIPS_DI,
    parse_IntegerRT,    //	T_MIPS_EI,

    parse_IntegerRD,    //	T_MIPS_MFHI,
    parse_IntegerRD,    //	T_MIPS_MFLO,

    parse_IntegerRS,    //	T_MIPS_JR,
    parse_IntegerRS,    //	T_MIPS_JR_HB,
    parse_IntegerRS,    //	T_MIPS_MTHI,
    parse_IntegerRS,    //	T_MIPS_MTLO,

    parse_IntegerJumpAbs,    //	T_MIPS_J,
    parse_IntegerJumpAbs,    //	T_MIPS_JAL,

    parse_LUI    //	T_MIPS_LUI
};

bool
mips_ParseIntegerInstruction(void) {
    if (T_MIPS_ADD <= lex_Current.token && lex_Current.token <= T_MIPS_LUI) {
        return s_mnemonicHandlers[lex_Current.token - T_MIPS_ADD]();
    }

    return false;
}
