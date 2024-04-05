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

	{ "ZERO", T_V_REG_X0  },
	{ "R0",   T_V_REG_X0  },
	{ "R1",   T_V_REG_X1  },
	{ "R2",   T_V_REG_X2  },
	{ "R3",   T_V_REG_X3  },
	{ "R4",   T_V_REG_X4  },
	{ "R5",   T_V_REG_X5  },
	{ "R6",   T_V_REG_X6  },
	{ "R7",   T_V_REG_X7  },
	{ "R8",   T_V_REG_X8  },
	{ "R9",   T_V_REG_X9  },
	{ "R10",  T_V_REG_X10  },
	{ "R11",  T_V_REG_X11  },
	{ "R12",  T_V_REG_X12  },
	{ "R13",  T_V_REG_X13  },
	{ "R14",  T_V_REG_X14  },
	{ "R15",  T_V_REG_X15  },
	{ "R16",  T_V_REG_X16  },
	{ "R17",  T_V_REG_X17  },
	{ "R18",  T_V_REG_X18  },
	{ "R19",  T_V_REG_X19  },
	{ "R20",  T_V_REG_X20  },
	{ "R21",  T_V_REG_X21  },
	{ "R22",  T_V_REG_X22  },
	{ "R23",  T_V_REG_X23  },
	{ "R24",  T_V_REG_X24  },
	{ "R25",  T_V_REG_X25  },
	{ "R26",  T_V_REG_X26  },
	{ "R27",  T_V_REG_X27  },
	{ "R28",  T_V_REG_X28  },
	{ "R29",  T_V_REG_X29  },
	{ "R30",  T_V_REG_X30  },
	{ "R31",  T_V_REG_X31  },

	{ NULL,   0 }
};


void 
v_DefineTokens(void) {
	lex_ConstantsDefineWords(g_tokens);
}
