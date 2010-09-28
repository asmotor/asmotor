/*  Copyright 2008 Carsten S�rensen

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

#ifndef	INCLUDE_OBJECT_H
#define	INCLUDE_OBJECT_H

extern BOOL obj_Write(char* name);


enum
{
	OBJ_OP_SUB,
	OBJ_OP_ADD,
	OBJ_OP_XOR,
	OBJ_OP_OR,
	OBJ_OP_AND,
	OBJ_OP_SHL,
	OBJ_OP_SHR,
	OBJ_OP_MUL,
	OBJ_OP_DIV,
	OBJ_OP_MOD,
	OBJ_OP_LOGICOR,
	OBJ_OP_LOGICAND,
	OBJ_OP_LOGICNOT,
	OBJ_OP_LOGICGE,
	OBJ_OP_LOGICGT,
	OBJ_OP_LOGICLE,
	OBJ_OP_LOGICLT,
	OBJ_OP_LOGICEQU,
	OBJ_OP_LOGICNE,
	OBJ_FUNC_LOWLIMIT,
	OBJ_FUNC_HIGHLIMIT,
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
	OBJ_PCREL,
#ifdef	HASBANKS
	OBJ_FUNC_BANK,
#endif
};

#endif	/*INCLUDE_OBJECT_H*/