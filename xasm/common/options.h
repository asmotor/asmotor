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

#include "locopt.h"

typedef	enum
{
	ASM_LITTLE_ENDIAN=0,
	ASM_BIG_ENDIAN=1
}	eEndian;

#define MAXDISABLEDWARNINGS 16

struct	Options
{
	list_Data(struct Options);
	ULONG	Flags;
	eEndian	Endian;
	char	BinaryChar[2];
	int		UninitChar;
	int		nTotalDisabledWarnings;
	UWORD	aDisabledWarnings[MAXDISABLEDWARNINGS];
	sMachineOptions Machine;
};
typedef	struct Options	sOptions;

extern	void	opt_Push(void);
extern	void	opt_Pop(void);
extern	void	opt_Parse(char* s);
extern	void	opt_Open(void);
extern	void	opt_Close(void);

extern	sOptions* pOptions;

#endif	/*INCLUDE_OPTIONS_H*/