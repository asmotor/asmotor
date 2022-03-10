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

#ifndef XASM_COMMON_ERRORS_H_INCLUDED_
#define XASM_COMMON_ERRORS_H_INCLUDED_

#include "patch.h"

typedef enum {
    WARN_CANNOT_PURGE = 0,
    WARN_OPTION,
    WARN_OPTION_POP,
    WARN_USER_GENERIC,
    WARN_SHIFT_MACRO,
    WARN_MEXIT_OUTSIDE_MACRO,
    WARN_REXIT_OUTSIDE_REPT,
    WARN_MACHINE_UNKNOWN_OPTION,
	WARN_SYMBOL_WITH_RESERVED_NAME,

    ERROR_CHAR_EXPECTED = 100,
    ERROR_EXPRESSION_N_BIT,
    ERROR_INVALID_EXPRESSION,
    ERROR_SOURCE_OPERAND,
    ERROR_DEST_OPERAND,
    ERROR_FIRST_OPERAND,
    ERROR_SECOND_OPERAND,
    ERROR_OPERAND,
    ERROR_EXPECT_EXPR,
    ERROR_OPERAND_RANGE,
    ERROR_MODIFY_SYMBOL,
    ERROR_LABEL_SECTION,
    ERROR_SYMBOL_EXPORT,
    ERROR_SECTION_DATA,
    ERROR_IMPORT_DEFINED,
    ERROR_SECTION_FULL,
    ERROR_SECTION_MISSING,
    ERROR_EXPR_CONST_RELOC,
    ERROR_EXPR_TWO_POWER,
    ERROR_EXPR_CONST,
    ERROR_EXPR_RELOC,
    ERROR_EXPR_STRING,
    ERROR_EXPR_BAD,
    ERROR_EXPECT_BANK,
    ERROR_EXPECT_TEXT_BSS,
    ERROR_IDENTIFIER_GROUP,
    ERROR_EXPECT_IDENTIFIER,
    ERROR_EXPR_POSITIVE,
    ERROR_SYNTAX,
    ERROR_INSTR_UNKNOWN,
    ERROR_SECTION_MUST_LOAD,
    ERROR_SECTION_LOAD,
    ERROR_SYMBOL_CONSTANT,
    ERROR_SYMBOL_EQUS,
    ERROR_SYMBOL_EQUF,
    ERROR_SECT_EXISTS,
    ERROR_READ,
    ERROR_NO_FILE,
    ERROR_unused_0,
    ERROR_unused_1,
    ERROR_unused_2,
    ERROR_NO_SECT,
    ERROR_ZERO_DIVIDE,
    ERROR_SYMBOL_IN_EXPR,
    ERROR_DEF_SYMBOL,
    ERROR_BANK_SYMBOL,
    ERROR_NEED_ENDM,
    ERROR_NEED_ENDR,
    ERROR_END_OF_FILE,
    ERROR_STRING_TERM,
    ERROR_ID_MALFORMED,
    ERROR_INCLUDE_LIMIT,
    ERROR_NO_MACRO,
    ERROR_SYMBOL_UNDEFINED,
    ERROR_OBJECTFILE_PATCH,
    ERROR_INVALID_MACRO_ARGUMENT,
    ERROR_STRING_MEMBER_NOT_INT,
    ERROR_TOO_MANY_FILES,
    ERROR_EXTENDED_PRECISION_UNSUPPORTED,
	ERROR_NEED_ENDC,
    ERROR_SECT_EXISTS_DIFFERENT_KIND,
    ERROR_SECT_FLAGS_COMBINATION,
	ERROR_GROUP_REDEFINED
} EError;

extern bool
err_Warn(uint32_t errorNumber, ...);

extern bool
err_Error(uint32_t errorNumber, ...);

extern bool
err_Fail(uint32_t errorNumber, ...);

extern bool
err_SymbolError(const SSymbol* patch, uint32_t errorNumber, ...);

extern bool
err_PatchError(const SPatch* patch, uint32_t errorNumber, ...);

extern bool
err_PatchFail(const SPatch* patch, uint32_t errorNumber, ...);

extern void
err_Suspend(void);

extern int
err_AcceptSuspended(void);

extern int
err_DiscardSuspended(void);

extern void
err_Init(void);

extern void
err_PrintAll(void);


#endif
