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

/*
 * xLink - OBJECT.C
 * Copyright 1996-1998 Carsten Sorensen (csorensen@ea.com)
 *
 *	char	ID[4]="XOB\1";
 *	[>=v1] char	MinimumWordSize ; Used for address calculations.
 *							; 1 - A CPU address points to a byte in memory
 *							; 2 - A CPU address points to a 16 bit word in memory (CPU address 0x1000 is the 0x2000th byte)
 *							; 4 - A CPU address points to a 32 bit word in memory (CPU address 0x1000 is the 0x4000th byte)
 *	uint32_t	NumberOfGroups
 *	REPT	NumberOfGroups
 *			ASCIIZ	Name
 *			uint32_t	Type
 *	ENDR
 *	uint32_t	NumberOfSections
 *	REPT	NumberOfSections
 *			int32_t	GroupID	; -1 = exported EQU symbols
 *			ASCIIZ	Name
 *			int32_t	Bank	; -1 = not bankfixed
 *			int32_t	Position; -1 = not fixed
 *			[>=v1] int32_t	BasePC	; -1 = not fixed
 *			uint32_t	NumberOfSymbols
 *			REPT	NumberOfSymbols
 *					ASCIIZ	Name
 *					uint32_t	Type	;0=EXPORT
 *									;1=IMPORT
 *									;2=LOCAL
 *									;3=LOCALEXPORT
 *									;4=LOCALIMPORT
 *					IF Type==EXPORT or LOCAL or LOCALEXPORT
 *						int32_t	Value
 *					ENDC
 *			ENDR
 *			uint32_t	Size
 *			IF	SectionCanContainData
 *					uint8_t	Data[Size]
 *					uint32_t	NumberOfPatches
 *					REPT	NumberOfPatches
 *							uint32_t	Offset
 *							uint32_t	Type
 *							uint32_t	ExprSize
 *							uint8_t	Expr[ExprSize]
 *					ENDR
 *			ENDC
 *	ENDR
 */

#include "xlink.h"

#include <string.h>

#define	MAKE_ID(a,b,c,d)	(a)|((b)<<8)|((c)<<16)|((d)<<24)

static uint32_t	FileID = 0;
static uint32_t s_nMinimumWordSize = 0;

static	uint32_t	fgetll(FILE* f)
{
	uint32_t	r;

	r =fgetc(f);
	r|=fgetc(f)<<8;
	r|=fgetc(f)<<16;
	r|=fgetc(f)<<24;

	return r;
}

static	void	fgetasciiz(char* s, int size, FILE* f)
{
	if(size > 0)
	{
		char ch;

		do
		{
			ch = *s++ = (char)fgetc(f);
			--size;
		} while(size != 0 && ch);
	}
}


static SGroups* read_groups(FILE* f)
{
/*
 *	uint32_t	NumberOfGroups
 *	REPT	NumberOfGroups
 *			ASCIIZ	Name
 *			uint32_t	Type
 *	ENDR
 */

	SGroups* pGroups;
	uint32_t	totalgroups;

	totalgroups=fgetll(f);

	if((pGroups=mem_Alloc(sizeof(SGroups)+totalgroups*sizeof(SGroup)))!=NULL)
	{
		uint32_t	i;

		pGroups->TotalGroups=totalgroups;

		for(i=0; i<totalgroups; i+=1)
		{
			fgetasciiz(pGroups->Groups[i].Name, MAXSYMNAMELENGTH, f);
			pGroups->Groups[i].Type=fgetll(f);
		}

	}
	else
	{
		Error("Out of memory");
	}

	return pGroups;
}

static	uint32_t	read_symbols(FILE* f, SSymbol* *pdestsym)
{
/*
 *			uint32_t	NumberOfSymbols
 *			REPT	NumberOfSymbols
 *					ASCIIZ	Name
 *					uint32_t	Type	;0=EXPORT
 *									;1=IMPORT
 *									;2=LOCAL
 *									;3=LOCALEXPORT
 *									;4=LOCALIMPORT
 *					IF Type==EXPORT or LOCAL or LOCALEXPORT
 *						int32_t	Value
 *					ENDC
 *			ENDR
 */

	uint32_t	totalsymbols;
	SSymbol* sym;

	totalsymbols=fgetll(f);

	if((sym=mem_Alloc(totalsymbols*sizeof(SSymbol)))!=NULL)
	{
		uint32_t	i;

		for(i=0; i<totalsymbols; i+=1)
		{
			fgetasciiz(sym[i].Name, MAXSYMNAMELENGTH, f);
			sym[i].Type=fgetll(f);
			if((sym[i].Type!=SYM_IMPORT) && (sym[i].Type!=SYM_LOCALIMPORT))
			{
				sym[i].Value=fgetll(f);
			}
			sym[i].Resolved=false;
		}

		*pdestsym=sym;
		return totalsymbols;
	}

	Error("Out of memory");
	return 0;
}

static	SPatches* read_patches(FILE* f)
{
	SPatches* patches;
	int			totalpatches;

	totalpatches=fgetll(f);

	if((patches=mem_Alloc(sizeof(SPatches)+totalpatches*sizeof(SPatch)))!=NULL)
	{
		int	i;

		patches->TotalPatches=totalpatches;

		for(i=0; i<totalpatches; i+=1)
		{
			patches->Patches[i].Offset=fgetll(f);
			patches->Patches[i].Type=fgetll(f);
			patches->Patches[i].ExprSize=fgetll(f);
			if((patches->Patches[i].pExpr=mem_Alloc(patches->Patches[i].ExprSize))!=NULL)
			{
				if(patches->Patches[i].ExprSize != fread(patches->Patches[i].pExpr, 1, patches->Patches[i].ExprSize, f))
					Error("File read failed");
			}
			else
			{
				Error("Out of memory");
			}
		}
		return patches;
	}

	Error("Out of memory");
	return NULL;
}

static void read_sections(SGroups* groups, FILE* f, int version)
{
/*
 *	uint32_t	NumberOfSections
 *	REPT	NumberOfSections
 *			int32_t	GroupID	; -1 = exported EQU symbols
 *			ASCIIZ	Name
 *			int32_t	Bank	; -1 = not bankfixed
 *			int32_t	Position; -1 = not fixed
 *			[>=v1] int32_t	BasePC	; -1 = not fixed
 *			uint32_t	NumberOfSymbols
 *			REPT	NumberOfSymbols
 *					ASCIIZ	Name
 *					uint32_t	Type	;0=EXPORT
 *									;1=IMPORT
 *									;2=LOCAL
 *									;3=LOCALEXPORT
 *									;4=LOCALIMPORT
 *					IF Type==EXPORT or LOCAL or LOCALEXPORT
 *						int32_t	Value
 *					ENDC
 *			ENDR
 *			uint32_t	Size
 *			IF	SectionCanContainData
 *					uint8_t	Data[Size]
 *					uint32_t	NumberOfPatches
 *					REPT	NumberOfPatches
 *							uint32_t	Offset
 *							uint32_t	Type
 *							uint32_t	ExprSize
 *							uint8_t	Expr[ExprSize]
 *					ENDR
 *			ENDC
 *	ENDR
 */

	uint32_t	totalsections;

	totalsections=fgetll(f);
	while(totalsections--)
	{
		SSection* section = sect_CreateNew();

		section->pGroups = groups;
		section->FileID = FileID;

		section->GroupID = fgetll(f);
		fgetasciiz(section->Name, MAXSYMNAMELENGTH, f);
		section->Bank = fgetll(f);
		section->Position = fgetll(f);
		if(version >= 1)
			section->BasePC = fgetll(f);
		else
			section->BasePC = section->Position;

		if(groups->Groups[section->GroupID].Type == GROUP_TEXT
		&& strcmp(groups->Groups[section->GroupID].Name, "HOME") == 0)
		{
			section->Bank = 0;
		}

		section->TotalSymbols=read_symbols(f, &section->pSymbols);

		section->Size=fgetll(f);

		if(section->GroupID >= 0 && groups->Groups[section->GroupID].Type == GROUP_TEXT)
		{
			if((section->pData = mem_Alloc(section->Size)) != NULL)
			{
				if(section->Size != fread(section->pData, 1, section->Size, f))
					Error("File read failed");
				section->pPatches = read_patches(f);
			}
		}
	}

	FileID+=1;
}

static	void	readchunk(FILE* f);

static	void	read_xob0(FILE* f)
{
	s_nMinimumWordSize = 1;
	read_sections(read_groups(f), f, 0);
}

static void read_xob1(FILE* f)
{
	s_nMinimumWordSize = fgetc(f);
	read_sections(read_groups(f), f, 1);
}

static	void	read_xlb0(FILE* f)
{
	uint32_t	count;

	count=fgetll(f);

	while(count--)
	{
		while(fgetc(f)){}	//	Skip name
		fgetll(f);		//	Skip time
		fgetll(f);		//	Skip date
		fgetll(f);		//	Skip length

		//read_sections(read_groups(f), f);
		readchunk(f);
	}
}


static	void	readchunk(FILE* f)
{
	uint32_t	id;

	id=fgetll(f);

	switch(id)
	{
		case	MAKE_ID('X','O','B',0):
		{
			read_xob0(f);
			break;
		}
		case	MAKE_ID('X','O','B',1):
		{
			read_xob1(f);
			break;
		}
		case	MAKE_ID('X','L','B',0):
		{
			read_xlb0(f);
			break;
		}
	}
}

static	long	filesize(FILE* f)
{
	long	pos,
			r;

	pos=ftell(f);
	fseek(f, 0, SEEK_END);
	r=ftell(f);
	fseek(f, pos, SEEK_SET);

	return r;
}

void	obj_Read(char* s)
{
	FILE* f;

	if((f=fopen(s,"rb"))!=NULL)
	{
		long	size;

		size=filesize(f);

		while(ftell(f)<size)
		{
			readchunk(f);
		}
		fclose(f);
	}
	else
	{
		Error("File \"%s\" not found", s);
	}
}
