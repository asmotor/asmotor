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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xasm.h"
#include "mem.h"
#include "options.h"
#include "localasm.h"
#include "locopt.h"
#include "loccpu.h"
#include "project.h"
#include "loclexer.h"

static int s_nPreviousInstructionSet = 0;

void locopt_Copy(struct MachineOptions* pDest, struct MachineOptions* pSrc)
{
	*pDest = *pSrc;
}

struct MachineOptions* locopt_Alloc(void)
{
	return mem_Alloc(sizeof(SMachineOptions));
}

void locopt_Open(void)
{
}

void locopt_Update(void)
{
}

bool_t locopt_Parse(char* s)
{
	if(s == NULL || strlen(s) == 0)
		return false;

	prj_Warn(WARN_MACHINE_UNKNOWN_OPTION, s);
	return false;
}

void locopt_PrintOptions(void)
{
}
