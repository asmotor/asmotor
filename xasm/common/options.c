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
#include "xasm.h"
#include "mem.h"
#include "options.h"
#include "lexer.h"
#include "tokens.h"
#include "project.h"
#include "filestack.h"

extern void
locopt_Update(void);

extern bool
locopt_Parse(char*);

extern void
locopt_Open(void);

SOptions* opt_Current;

static void
opt_Update(void) {
    lex_VariadicRemoveAll(tokens_BinaryVariadicId);

    lex_VariadicAddCharRange(tokens_BinaryVariadicId, '%', '%', 0);
    lex_VariadicAddCharRangeRepeating(tokens_BinaryVariadicId, opt_Current->binaryLiteralCharacters[0], opt_Current->binaryLiteralCharacters[0], 1);
    lex_VariadicAddCharRangeRepeating(tokens_BinaryVariadicId, opt_Current->binaryLiteralCharacters[1], opt_Current->binaryLiteralCharacters[1], 1);

    locopt_Update();
}

static SOptions*
opt_Alloc(void) {
    SOptions* nopt = (SOptions*) mem_Alloc(sizeof(SOptions));
    memset(nopt, 0, sizeof(SOptions));
    nopt->machineOptions = locopt_Alloc();
    return nopt;
}

static void
opt_Copy(SOptions* pDest, SOptions* pSrc) {
    struct MachineOptions* p = pDest->machineOptions;

    *pDest = *pSrc;
    pDest->machineOptions = p;

    locopt_Copy(pDest->machineOptions, pSrc->machineOptions);
}

void
opt_Push(void) {
    SOptions* nopt = opt_Alloc();
    opt_Copy(nopt, opt_Current);

    list_Insert(opt_Current, nopt);
}

void
opt_Pop(void) {
    if (!list_isLast(opt_Current)) {
        SOptions* nopt = opt_Current;

        list_Remove(opt_Current, opt_Current);
        mem_Free(nopt);
        opt_Update();
    } else {
        prj_Warn(WARN_OPTION_POP);
    }
}

void
opt_Parse(char* s) {
    switch (s[0]) {
        case 'w': {
            int w;
            if (opt_Current->disabledWarningsCount < MAX_DISABLED_WARNINGS && 1 == sscanf(&s[1], "%d", &w)) {
                opt_Current->disabledWarnings[opt_Current->disabledWarningsCount++] = (uint16_t) w;
            } else {
                prj_Warn(WARN_OPTION, s);
            }
            break;
        }
        case 'i': {
            string* pPath = str_Create(&s[1]);
            fstk_AddIncludePath(pPath);
            str_Free(pPath);
            break;
        }
        case 'e': {
            switch (s[1]) {
                case 'b':
                    opt_Current->endianness = ASM_BIG_ENDIAN;
                    break;
                case 'l':
                    opt_Current->endianness = ASM_LITTLE_ENDIAN;
                    break;
                default:
                    prj_Warn(WARN_OPTION, s);
                    break;
            }
            break;
        }
        case 'm': {
            locopt_Parse(&s[1]);
            break;
        }
        case 'b': {
            if (strlen(&s[1]) == 2) {
                opt_Current->binaryLiteralCharacters[0] = (uint8_t) s[1];
                opt_Current->binaryLiteralCharacters[1] = (uint8_t) s[2];
            } else {
                prj_Warn(WARN_OPTION, s);
            }
            break;
        }
        case 'z': {
            if (strlen(&s[1]) <= 2) {
                if (strcmp(&s[1], "?") == 0) {
                    opt_Current->uninitializedValue = 0xFF;
                } else {
                    int uninitializedValue;
                    int result = sscanf(&s[1], "%x", &uninitializedValue);
                    opt_Current->uninitializedValue = (uint8_t) uninitializedValue;
                    if (result == EOF || result != 1) {
                        prj_Warn(WARN_OPTION, s);
                    }
                }
            } else {
                prj_Warn(WARN_OPTION, s);
            }
            break;
        }
        default: {
            prj_Warn(WARN_OPTION, s);
            break;
        }
    }
    opt_Update();
}

void
opt_Open(void) {
    opt_Current = opt_Alloc();

    opt_Current->endianness = g_pConfiguration->eDefaultEndianness;
    opt_Current->binaryLiteralCharacters[0] = '0';
    opt_Current->binaryLiteralCharacters[1] = '1';
    opt_Current->uninitializedValue = 0xFF;
    opt_Current->disabledWarningsCount = 0;
    opt_Current->allowReservedKeywordLabels = true;
    locopt_Open();
    opt_Update();
}

void
opt_Close(void) {
    while (opt_Current != NULL) {
        SOptions* t = opt_Current;
        list_Remove(opt_Current, opt_Current);
        mem_Free(t);
    }
}
