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

#include <assert.h>
#include <stdbool.h>

#include "expression.h"
#include "lexer.h"
#include "parse.h"
#include "errors.h"

#include "m6809_errors.h"
#include "m6809_parse.h"
#include "m6809_tokens.h"

typedef struct Parser {
    uint8_t baseOpcode;
    uint32_t allowedModes;
    bool (*handler)(uint8_t baseOpcode, SAddressingMode* addrMode);
} SParser;

static bool
handleImplied(uint8_t baseOpcode, SAddressingMode* addrMode) {
    sect_OutputConst8(baseOpcode);
    return true;
}

static SParser g_instructionHandlers[T_6809_NOP - T_6809_NOP + 1] = {
    { 0x12, 0, handleImplied },	/* NOP */
};

bool
m6809_ParseIntegerInstruction(void) {
    if (T_6809_NOP <= lex_Context->token.id && lex_Context->token.id <= T_6809_NOP) {
        SAddressingMode addrMode;
        ETargetToken token = (ETargetToken) lex_Context->token.id;
        SParser* handler = &g_instructionHandlers[token - T_6809_NOP];

        parse_GetToken();
        if (m6809_ParseAddressingMode(&addrMode, handler->allowedModes))
            return handler->handler(handler->baseOpcode, &addrMode);
        else
            err_Error(MERROR_ILLEGAL_ADDRMODE);
    }

    return false;
}
