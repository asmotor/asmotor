/*  Copyright 2008 Carsten Sørensen

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

#ifndef	INCLUDE_TOKENS_H
#define	INCLUDE_TOKENS_H

typedef	enum
{
    T_LEFT_PARENS = '(',
    T_AT = '@',

	T_FIRST_TOKEN = 300,
	T_STRING = T_FIRST_TOKEN,
	T_LABEL,
	T_ID,
	T_NUMBER,

	T_OP_LOGICOR,
	T_OP_LOGICAND,
	T_OP_LOGICEQU,
	T_OP_LOGICGT,
	T_OP_LOGICLT,
	T_OP_LOGICGE,
	T_OP_LOGICLE,
	T_OP_LOGICNE,
	T_OP_LOGICNOT,
	T_OP_OR,
	T_OP_XOR,
	T_OP_AND,
	T_OP_SHL,
	T_OP_SHR,
	T_OP_ADD,
	T_OP_SUB,
	T_OP_MUL,
	T_OP_DIV,
	T_OP_MOD,
	T_OP_NOT,
	T_OP_BIT,

	T_FUNC_DEF,

	T_FUNC_BANK,

    T_FUNC_FDIV,
    T_FUNC_FMUL,
    T_FUNC_SIN,
    T_FUNC_COS,
    T_FUNC_TAN,
    T_FUNC_ASIN,
    T_FUNC_ACOS,
    T_FUNC_ATAN,
    T_FUNC_ATAN2,

    T_FUNC_COMPARETO,
    T_FUNC_INDEXOF,
    T_FUNC_SLICE,
    T_FUNC_LENGTH,
    T_FUNC_TOUPPER,
    T_FUNC_TOLOWER,

    T_POP_RB,
    T_POP_RW,
    T_POP_RL,

	T_POP_EQU,
    T_POP_EQUS,
    T_POP_SET,

    T_POP_PRINTT,
    T_POP_PRINTV,
    T_POP_PRINTF,
    T_POP_EXPORT,
    T_POP_IMPORT,
    T_POP_GLOBAL,
	T_POP_RSRESET,
	T_POP_RSSET,

	T_POP_FAIL,
	T_POP_WARN,

	T_POP_PURGE,

    T_POP_INCLUDE,

	T_POP_DSB,
	T_POP_DSW,
	T_POP_DSL,

	T_POP_DB,
    T_POP_DW,
    T_POP_DL,

	T_POP_SECTION,

	T_POP_INCBIN,

	T_POP_MACRO,
	T_POP_ENDM,		/* Not needed but we have it here just to protect the name */
	T_POP_SHIFT,
	T_POP_MEXIT,

	T_POP_REPT,
	T_POP_ENDR,		/* Not needed but we have it here just to protect the name */
	T_POP_REXIT,

	T_POP_IF,
	T_POP_IFC,
	T_POP_IFD,
	T_POP_IFNC,
	T_POP_IFND,
	T_POP_IFEQ,
	T_POP_IFGT,
	T_POP_IFGE,
	T_POP_IFLT,
	T_POP_IFLE,
	T_POP_ELSE,
	T_POP_ENDC,

	T_POP_GROUP,
	T_GROUP_TEXT,
	T_GROUP_BSS,

	T_POP_PUSHS,
	T_POP_POPS,
	T_POP_PUSHO,
	T_POP_POPO,

	T_POP_OPT,

	T_FUNC_LOWLIMIT,
	T_FUNC_HIGHLIMIT,

	T_MACROARG0,
	
	T_POP_CNOP,
	T_POP_EVEN,
	T_POP_END
} EToken;

#endif	/*INCLUDE_TOKENS_H*/
