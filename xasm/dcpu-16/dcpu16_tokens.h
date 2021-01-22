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

#ifndef XASM_DCPU16_TOKENS_H_INCLUDED_
#define XASM_DCPU16_TOKENS_H_INCLUDED_

typedef enum {
    T_DCPU16_ADD = 6000,
    T_DCPU16_AND,
    T_DCPU16_BOR,
    T_DCPU16_DIV,
    T_DCPU16_IFB,
    T_DCPU16_IFE,
    T_DCPU16_IFG,
    T_DCPU16_IFN,
    T_DCPU16_JSR,
    T_DCPU16_MOD,
    T_DCPU16_MUL,
    T_DCPU16_SET,
    T_DCPU16_SHL,
    T_DCPU16_SHR,
    T_DCPU16_SUB,
    T_DCPU16_XOR,

    T_REG_A,
    T_REG_B,
    T_REG_C,
    T_REG_X,
    T_REG_Y,
    T_REG_Z,
    T_REG_I,
    T_REG_J,
    T_REG_POP,
    T_REG_PEEK,
    T_REG_PUSH,
    T_REG_SP,
    T_REG_PC,
    T_REG_O
} ETargetToken;

extern void
x10c_DefineTokens(void);

#endif
