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

#ifndef	INCLUDE_SECTION_H
#define	INCLUDE_SECTION_H

#include "lists.h"
#include "xasm.h"

struct Patch;
struct Symbol;
struct Expression;

struct Section
{
	list_Data(struct Section);
	char	Name[MAXSYMNAMELENGTH + 1];
	struct Symbol* pGroup;
	ULONG	PC;
	ULONG	Flags;
	ULONG	UsedSpace;		/*	How many bytes are used in the section */
	ULONG	FreeSpace;		/*	How many bytes are free */
	ULONG	AllocatedSpace;	/*	How big a chunk of memory pData is pointing to */
	SLONG	Org;
	SLONG	Bank;
	struct Patch* pPatches;
	UBYTE* pData;
};
typedef	struct Section SSection;

#define	SECTF_ORGFIXED	0x01
#define	SECTF_BANKFIXED	0x02

extern	SSection* pCurrentSection;
extern	SSection* pSectionList;

extern BOOL	sect_SwitchTo(char* sectname, struct Symbol* group);
extern BOOL	sect_SwitchTo_ORG(char* sectname, struct Symbol* group, SLONG org);
extern BOOL	sect_SwitchTo_BANK(char* sectname, struct Symbol* group, SLONG bank);
extern BOOL	sect_SwitchTo_ORG_BANK(char* sectname, struct Symbol* group, SLONG org, SLONG bank);
extern BOOL	sect_SwitchTo_NAMEONLY(char* sectname);
extern BOOL	sect_Init(void);
extern void	sect_SkipBytes(SLONG count);
extern void	sect_Align(SLONG align);
extern void	sect_OutputExprByte(struct Expression* expr);
extern void	sect_OutputExprWord(struct Expression* expr);
extern void	sect_OutputExprLong(struct Expression* expr);
extern void	sect_OutputBinaryFile(char* s);
extern void	sect_OutputAbsByte(UBYTE value);
extern void	sect_OutputAbsWord(UWORD value);
extern void	sect_OutputAbsLong(ULONG value);

#endif	/*INCLUDE_SECTION_H*/