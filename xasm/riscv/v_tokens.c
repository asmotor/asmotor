/*  Copyright 2008-2022 Carsten Elton Sorensen and contributors

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

#include "lexer_constants.h"

#include "v_tokens.h"

static SLexConstantsWord 
g_tokens[] = {
	{ "ADD",  T_V_ADD    },

	{ "X0",   T_V_REG_X0  },
	{ "X1",   T_V_REG_X1  },
	{ "X2",   T_V_REG_X2  },
	{ "X3",   T_V_REG_X3  },
	{ "X4",   T_V_REG_X4  },
	{ "X5",   T_V_REG_X5  },
	{ "X6",   T_V_REG_X6  },
	{ "X7",   T_V_REG_X7  },
	{ "X8",   T_V_REG_X8  },
	{ "X9",   T_V_REG_X9  },
	{ "X10",  T_V_REG_X10  },
	{ "X11",  T_V_REG_X11  },
	{ "X12",  T_V_REG_X12  },
	{ "X13",  T_V_REG_X13  },
	{ "X14",  T_V_REG_X14  },
	{ "X15",  T_V_REG_X15  },
	{ "X16",  T_V_REG_X16  },
	{ "X17",  T_V_REG_X17  },
	{ "X18",  T_V_REG_X18  },
	{ "X19",  T_V_REG_X19  },
	{ "X20",  T_V_REG_X20  },
	{ "X21",  T_V_REG_X21  },
	{ "X22",  T_V_REG_X22  },
	{ "X23",  T_V_REG_X23  },
	{ "X24",  T_V_REG_X24  },
	{ "X25",  T_V_REG_X25  },
	{ "X26",  T_V_REG_X26  },
	{ "X27",  T_V_REG_X27  },
	{ "X28",  T_V_REG_X28  },
	{ "X29",  T_V_REG_X29  },
	{ "X30",  T_V_REG_X30  },
	{ "X31",  T_V_REG_X31  },

	{ "ZERO",   T_V_REG_X0  },
	{ "RA",   T_V_REG_X1  },
	{ "SP",   T_V_REG_X2  },
	{ "GP",   T_V_REG_X3  },
	{ "TP",   T_V_REG_X4  },
	{ "T0",   T_V_REG_X5  },
	{ "T1",   T_V_REG_X6  },
	{ "T2",   T_V_REG_X7  },
	{ "S0",   T_V_REG_X8  },
	{ "FP",   T_V_REG_X8  },
	{ "S1",   T_V_REG_X9  },
	{ "A0",  T_V_REG_X10  },
	{ "A1",  T_V_REG_X11  },
	{ "A2",  T_V_REG_X12  },
	{ "A3",  T_V_REG_X13  },
	{ "A4",  T_V_REG_X14  },
	{ "A5",  T_V_REG_X15  },
	{ "A6",  T_V_REG_X16  },
	{ "A7",  T_V_REG_X17  },
	{ "S2",  T_V_REG_X18  },
	{ "S3",  T_V_REG_X19  },
	{ "S4",  T_V_REG_X20  },
	{ "S5",  T_V_REG_X21  },
	{ "S6",  T_V_REG_X22  },
	{ "S7",  T_V_REG_X23  },
	{ "S8",  T_V_REG_X24  },
	{ "S9",  T_V_REG_X25  },
	{ "S10",  T_V_REG_X26  },
	{ "S11",  T_V_REG_X27  },
	{ "T3",  T_V_REG_X28  },
	{ "T4",  T_V_REG_X29  },
	{ "T5",  T_V_REG_X30  },
	{ "T6",  T_V_REG_X31  },

	{ NULL,   0 }
};


void 
v_DefineTokens(void) {
	lex_ConstantsDefineWords(g_tokens);
}
