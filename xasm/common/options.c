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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "asmotor.h"
#include "mem.h"

#include "xasm.h"
#include "errors.h"
#include "filestack.h"
#include "includes.h"
#include "lexer_variadics.h"
#include "options.h"
#include "tokens.h"

SOptions* opt_Current;

static bool
handleDisableWarning(const char* warning) {
    int warningInt;
    if (opt_Current->disabledWarningsCount < MAX_DISABLED_WARNINGS && decimalToInt(warning, &warningInt)) {
        opt_Current->disabledWarnings[opt_Current->disabledWarningsCount++] = (uint16_t) warningInt;
        return true;
    }
    
    return false;
}

static void
handleAddIncludePath(const char* path) {
    string* pathString = str_Create(path);
    inc_AddIncludePath(pathString);
    str_Free(pathString);
}

static bool
handleEndianness(char endianness) {
    switch (endianness) {
        case 'b':
            opt_Current->endianness = ASM_BIG_ENDIAN;
            return true;
        case 'l':
            opt_Current->endianness = ASM_LITTLE_ENDIAN;
            return true;
        default:
            return false;
    }
}

static bool
handleBinaryLiteralChars(const char* characters) {
    if (strlen(characters) == 2) {
        opt_Current->binaryLiteralCharacters[0] = (uint8_t) characters[0];
        opt_Current->binaryLiteralCharacters[1] = (uint8_t) characters[1];
        return true;
    }

    return false;
}

static bool
handleUnitializedFill(const char* fill) {
    if (strlen(fill) <= 2) {
        if (strcmp(fill, "?") == 0) {
            opt_Current->uninitializedValue = 0xFF;
            return true;
        } else {
            uint32_t uninitializedValue;
            bool success = hexToInt(fill, &uninitializedValue);
            opt_Current->uninitializedValue = (uint8_t) uninitializedValue;
            return success;
        }
    }

    return false;
}

static SOptions*
allocOptions(void) {
    SOptions* nopt = (SOptions*) mem_Alloc(sizeof(SOptions));
    memset(nopt, 0, sizeof(SOptions));
    nopt->machineOptions = xasm_Configuration->allocOptions();
    return nopt;
}

static void
copyOptions(SOptions* pDest, SOptions* pSrc) {
    struct MachineOptions* p = pDest->machineOptions;

    *pDest = *pSrc;
    pDest->machineOptions = p;

    xasm_Configuration->copyOptions(pDest->machineOptions, pSrc->machineOptions);
}

void
opt_Push(void) {
    SOptions* nopt = allocOptions();
    copyOptions(nopt, opt_Current);

    list_Insert(opt_Current, nopt);
}

void
opt_Pop(void) {
    if (!list_IsLast(opt_Current)) {
        SOptions* nopt = opt_Current;

        list_Remove(opt_Current, opt_Current);
        mem_Free(nopt);
        opt_Updated();
    } else {
        err_Warn(WARN_OPTION_POP);
    }
}


void
opt_Parse(char* option) {
    switch (option[0]) {
        case 'a': {
            if (!decimalToInt(&option[1], &opt_Current->sectionAlignment))
                err_Warn(WARN_OPTION, option);
            break;
        }
        case 'b': {
            if (!handleBinaryLiteralChars(&option[1]))
                err_Warn(WARN_OPTION, option);
            break;
        }
        case 'e': {
            if (!handleEndianness(option[1]))
                err_Warn(WARN_OPTION, option);
            break;
        }
        case 'g': {
            opt_Current->enableDebugInfo = true;
            break;
        }
        case 'i': {
            handleAddIncludePath(&option[1]);
            break;
        }
        case 'm': {
            xasm_Configuration->parseOption(&option[1]);
            break;
        }
        case 'w': {
            if (!handleDisableWarning(&option[1]))
                err_Warn(WARN_OPTION, option);
            break;
        }
        case 'z': {
            if (!handleUnitializedFill(&option[1]))
                err_Warn(WARN_OPTION, option);
            break;
        }
        default: {
            err_Warn(WARN_OPTION, option);
            break;
        }
    }
}

void
opt_Open(void) {
    opt_Current = allocOptions();

    opt_Current->endianness = xasm_Configuration->defaultEndianness;
    opt_Current->binaryLiteralCharacters[0] = '0';
    opt_Current->binaryLiteralCharacters[1] = '1';
    opt_Current->uninitializedValue = 0xFF;
    opt_Current->sectionAlignment = xasm_Configuration->sectionAlignment;
    opt_Current->disabledWarningsCount = 0;
    opt_Current->allowReservedKeywordLabels = true;
    opt_Current->enableDebugInfo = false;

    xasm_Configuration->setDefaultOptions(opt_Current->machineOptions);
    opt_Updated();
}

void
opt_Close(void) {
    while (opt_Current != NULL) {
        SOptions* t = opt_Current;
        list_Remove(opt_Current, opt_Current);
        mem_Free(t);
    }
}

extern void
opt_Updated(void) {
    lex_VariadicRemoveAll(tokens_BinaryVariadicId);

    lex_VariadicAddCharRange(tokens_BinaryVariadicId, '%', '%', 0);
    lex_VariadicAddCharRangeRepeating(tokens_BinaryVariadicId, opt_Current->binaryLiteralCharacters[0], opt_Current->binaryLiteralCharacters[0], 1);
    lex_VariadicAddCharRangeRepeating(tokens_BinaryVariadicId, opt_Current->binaryLiteralCharacters[1], opt_Current->binaryLiteralCharacters[1], 1);

    xasm_Configuration->onOptionsUpdated(opt_Current->machineOptions);
}

