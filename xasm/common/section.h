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
	uint32_t	PC;
	uint32_t	Flags;
	uint32_t	UsedSpace;		/*	How many bytes are used in the section */
	uint32_t	FreeSpace;		/*	How many bytes are free */
	uint32_t	AllocatedSpace;	/*	How big a chunk of memory pData is pointing to */
	int32_t	Org;
	int32_t	Bank;
	struct Patch* pPatches;
	uint8_t* pData;
};
typedef	struct Section SSection;

#define	SECTF_ORGFIXED	0x01
#define	SECTF_BANKFIXED	0x02

extern	SSection* pCurrentSection;
extern	SSection* pSectionList;

extern bool_t	sect_SwitchTo(char* sectname, struct Symbol* group);
extern bool_t	sect_SwitchTo_ORG(char* sectname, struct Symbol* group, int32_t org);
extern bool_t	sect_SwitchTo_BANK(char* sectname, struct Symbol* group, int32_t bank);
extern bool_t	sect_SwitchTo_ORG_BANK(char* sectname, struct Symbol* group, int32_t org, int32_t bank);
extern bool_t	sect_SwitchTo_NAMEONLY(char* sectname);
extern bool_t	sect_Init(void);
extern void	sect_SkipBytes(int32_t count);
extern void	sect_Align(int32_t align);
extern void	sect_OutputExpr8(struct Expression* expr);
extern void	sect_OutputExpr16(struct Expression* expr);
extern void	sect_OutputExprLong(struct Expression* expr);
extern void	sect_OutputBinaryFile(char* s);
extern void	sect_OutputConst8(uint8_t value);
extern void	sect_OutputConst16(uint16_t value);
extern void	sect_OutputConst32(uint32_t value);

#endif	/*INCLUDE_SECTION_H*/