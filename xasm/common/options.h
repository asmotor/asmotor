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

#ifndef	INCLUDE_OPTIONS_H
#define	INCLUDE_OPTIONS_H

#include "lists.h"
#include "xasm.h"


#define MAXDISABLEDWARNINGS 16

struct MachineOptions;

struct Options
{
	list_Data(struct Options);
	ULONG	Flags;
	EEndian	Endian;
	char	BinaryChar[2];
	int		UninitChar;
	int		nTotalDisabledWarnings;
	UWORD	aDisabledWarnings[MAXDISABLEDWARNINGS];
	struct MachineOptions*	pMachine;
};
typedef	struct Options SOptions;

extern void	opt_Push(void);
extern void	opt_Pop(void);
extern void	opt_Parse(char* s);
extern void	opt_Open(void);
extern void	opt_Close(void);

extern SOptions* g_pOptions;

#endif	/*INCLUDE_OPTIONS_H*/