/*  Copyright 2008-2021 Carsten Elton Sorensen and contributors

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

#ifndef XLINK_PATCH_H_INCLUDED_
#define XLINK_PATCH_H_INCLUDED_

typedef enum {
    PATCH_8,
    PATCH_LE_16,
    PATCH_BE_16,
    PATCH_LE_32,
    PATCH_BE_32,
    PATCH_NONE,
    PATCH_RELOC
} EPatchType;

typedef enum {
    OBJ_OP_SUB,
    OBJ_OP_ADD,
    OBJ_OP_XOR,
    OBJ_OP_OR,
    OBJ_OP_AND,
    OBJ_OP_ASL,
    OBJ_OP_ASR,
    OBJ_OP_MUL,
    OBJ_OP_DIV,
    OBJ_OP_MOD,
    OBJ_OP_BOOLEAN_OR,
    OBJ_OP_BOOLEAN_AND,
    OBJ_OP_BOOLEAN_NOT,
    OBJ_OP_GREATER_OR_EQUAL,
    OBJ_OP_GREATER_THAN,
    OBJ_OP_LESS_OR_EQUAL,
    OBJ_OP_LESS_THAN,
    OBJ_OP_EQUALS,
    OBJ_OP_NOT_EQUALS,
    OBJ_FUNC_LOW_LIMIT,
    OBJ_FUNC_HIGH_LIMIT,
    OBJ_FUNC_FDIV,
    OBJ_FUNC_FMUL,
    OBJ_FUNC_ATAN2,
    OBJ_FUNC_SIN,
    OBJ_FUNC_COS,
    OBJ_FUNC_TAN,
    OBJ_FUNC_ASIN,
    OBJ_FUNC_ACOS,
    OBJ_FUNC_ATAN,
    OBJ_CONSTANT,
    OBJ_SYMBOL,
    OBJ_PC_REL,
    OBJ_FUNC_BANK,
} EExpressionOperator;

typedef struct {
    EPatchType type;

    struct Symbol* valueSymbol;
    struct Section* valueSection;

    uint32_t offset;

    uint32_t expressionSize;
    uint8_t* expression;
} SPatch;

typedef struct Patches {
    uint32_t totalPatches;
    SPatch patches[];
} SPatches;

extern void
patch_Process(bool allowReloc, bool onlySectionRelativeReloc, bool allowImports);

extern SPatches*
patch_Alloc(uint32_t totalPatches);

#endif
