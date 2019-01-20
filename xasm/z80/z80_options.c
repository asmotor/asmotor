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
#include <string.h>

#include "mem.h"

#include "xasm.h"
#include "lexer_variadics.h"
#include "options.h"
#include "project.h"

#include "z80_options.h"
#include "z80_tokens.h"

extern SConfiguration xasm_Z80Configuration;

uint32_t g_GameboyLiteralId;

SMachineOptions* locopt_Alloc(void)
{
    return mem_Alloc(sizeof(SMachineOptions));
}

void locopt_Copy(SMachineOptions* pDest, SMachineOptions* pSrc)
{
    *pDest = *pSrc;
}

void locopt_Open(void)
{
    opt_Current->machineOptions->GameboyChar[0] = '0';
    opt_Current->machineOptions->GameboyChar[1] = '1';
    opt_Current->machineOptions->GameboyChar[2] = '2';
    opt_Current->machineOptions->GameboyChar[3] = '3';
    opt_Current->machineOptions->cpu = CPUF_Z80;
}

void locopt_Update(void)
{
    lex_VariadicRemoveAll(g_GameboyLiteralId);
    lex_VariadicAddCharRange(g_GameboyLiteralId, '`', '`', 0);
    lex_VariadicAddCharRangeRepeating(g_GameboyLiteralId, (uint8_t) opt_Current->machineOptions->GameboyChar[0],
                                      (uint8_t) opt_Current->machineOptions->GameboyChar[0], 1);
    lex_VariadicAddCharRangeRepeating(g_GameboyLiteralId, (uint8_t) opt_Current->machineOptions->GameboyChar[1],
                                      (uint8_t) opt_Current->machineOptions->GameboyChar[1], 1);
    lex_VariadicAddCharRangeRepeating(g_GameboyLiteralId, (uint8_t) opt_Current->machineOptions->GameboyChar[2],
                                      (uint8_t) opt_Current->machineOptions->GameboyChar[2], 1);
    lex_VariadicAddCharRangeRepeating(g_GameboyLiteralId, (uint8_t) opt_Current->machineOptions->GameboyChar[3],
                                      (uint8_t) opt_Current->machineOptions->GameboyChar[3], 1);
}

bool locopt_Parse(char* s)
{
    if(s == NULL || strlen(s) == 0)
        return false;

    switch(s[0])
    {
        case 'g':
            if(strlen(&s[1])==4)
            {
                opt_Current->machineOptions->GameboyChar[0]=s[1];
                opt_Current->machineOptions->GameboyChar[1]=s[2];
                opt_Current->machineOptions->GameboyChar[2]=s[3];
                opt_Current->machineOptions->GameboyChar[3]=s[4];
                return true;
            }

            prj_Warn(WARN_MACHINE_UNKNOWN_OPTION, s);
            return false;
        case 'c':
            if(strlen(&s[1]) == 1)
            {
                switch(s[1])
                {
                    case 'g':
                        opt_Current->machineOptions->cpu = CPUF_GB;
                        xasm_Z80Configuration.maxSectionSize = 0x4000;
                        return true;
                    case 'z':
                        opt_Current->machineOptions->cpu = CPUF_Z80;
                        xasm_Z80Configuration.maxSectionSize = 0x10000;
                        return true;
                    default:
                        break;
                }
            }
            prj_Warn(WARN_MACHINE_UNKNOWN_OPTION, s);
            return false;
        default:
            prj_Warn(WARN_MACHINE_UNKNOWN_OPTION, s);
            return false;
    }
}

void locopt_PrintOptions(void)
{
    printf("    -mg<ASCI> Change the four characters used for Gameboy graphics\n"
           "              constants (default is 0123)\n");
    printf("    -mc<x>    Change CPU type:\n"
           "                  g - Gameboy\n"
           "                  z - Z80\n");
}
