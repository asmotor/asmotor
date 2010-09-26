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
 *	char	ID[4]="XOB\0";
 *	ULONG	NumberOfGroups
 *	REPT	NumberOfGroups
 *			ASCIIZ	Name
 *			ULONG	Type
 *	ENDR
 *	ULONG	NumberOfSections
 *	REPT	NumberOfSections
 *			SLONG	GroupID	; -1 = exported EQU symbols
 *			ASCIIZ	Name
 *			SLONG	Bank	; -1 = not bankfixed
 *			SLONG	Org		; -1 = not orgfixed
 *			ULONG	NumberOfSymbols
 *			REPT	NumberOfSymbols
 *					ASCIIZ	Name
 *					ULONG	Type	;0=EXPORT
 *									;1=IMPORT
 *									;2=LOCAL
 *									;3=LOCALEXPORT
 *									;4=LOCALIMPORT
 *					IF Type==EXPORT or LOCAL or LOCALEXPORT
 *						SLONG	Value
 *					ENDC
 *			ENDR
 *			ULONG	Size
 *			IF	SectionCanContainData
 *					UBYTE	Data[Size]
 *					ULONG	NumberOfPatches
 *					REPT	NumberOfPatches
 *							ULONG	Offset
 *							ULONG	Type
 *							ULONG	ExprSize
 *							UBYTE	Expr[ExprSize]
 *					ENDR
 *			ENDC
 *	ENDR
 */

#include "xlink.h"

#define	MAKE_ID(a,b,c,d)	(a)|((b)<<8)|((c)<<16)|((d)<<24)

static	ULONG	FileID=0;

static	ULONG	fgetll(FILE* f)
{
	ULONG	r;

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
 *	ULONG	NumberOfGroups
 *	REPT	NumberOfGroups
 *			ASCIIZ	Name
 *			ULONG	Type
 *	ENDR
 */

	SGroups* pGroups;
	ULONG	totalgroups;

	totalgroups=fgetll(f);

	if((pGroups=malloc(sizeof(SGroups)+totalgroups*sizeof(SGroup)))!=NULL)
	{
		ULONG	i;

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

static	ULONG	read_symbols(FILE* f, SSymbol* *pdestsym)
{
/*
 *			ULONG	NumberOfSymbols
 *			REPT	NumberOfSymbols
 *					ASCIIZ	Name
 *					ULONG	Type	;0=EXPORT
 *									;1=IMPORT
 *									;2=LOCAL
 *									;3=LOCALEXPORT
 *									;4=LOCALIMPORT
 *					IF Type==EXPORT or LOCAL or LOCALEXPORT
 *						SLONG	Value
 *					ENDC
 *			ENDR
 */

	ULONG	totalsymbols;
	SSymbol* sym;

	totalsymbols=fgetll(f);

	if((sym=malloc(totalsymbols*sizeof(SSymbol)))!=NULL)
	{
		ULONG	i;

		for(i=0; i<totalsymbols; i+=1)
		{
			fgetasciiz(sym[i].Name, MAXSYMNAMELENGTH, f);
			sym[i].Type=fgetll(f);
			if((sym[i].Type!=SYM_IMPORT) && (sym[i].Type!=SYM_LOCALIMPORT))
			{
				sym[i].Value=fgetll(f);
			}
			sym[i].Resolved=FALSE;
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

	if((patches=malloc(sizeof(SPatches)+totalpatches*sizeof(SPatch)))!=NULL)
	{
		int	i;

		patches->TotalPatches=totalpatches;

		for(i=0; i<totalpatches; i+=1)
		{
			patches->Patches[i].Offset=fgetll(f);
			patches->Patches[i].Type=fgetll(f);
			patches->Patches[i].ExprSize=fgetll(f);
			if((patches->Patches[i].pExpr=malloc(patches->Patches[i].ExprSize))!=NULL)
			{
				fread(patches->Patches[i].pExpr, 1, patches->Patches[i].ExprSize, f);
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

static	void	read_sections(SGroups* groups, FILE* f)
{
/*
 *	ULONG	NumberOfSections
 *	REPT	NumberOfSections
 *			SLONG	GroupID	; -1 = exported EQU symbols
 *			ASCIIZ	Name
 *			SLONG	Bank	; -1 = not bankfixed
 *			SLONG	Org		; -1 = not orgfixed
 *			ULONG	NumberOfSymbols
 *			REPT	NumberOfSymbols
 *					ASCIIZ	Name
 *					ULONG	Type	;0=EXPORT
 *									;1=IMPORT
 *									;2=LOCAL
 *									;3=LOCALEXPORT
 *									;4=LOCALIMPORT
 *					IF Type==EXPORT or LOCAL or LOCALEXPORT
 *						SLONG	Value
 *					ENDC
 *			ENDR
 *			ULONG	Size
 *			IF	SectionCanContainData
 *					UBYTE	Data[Size]
 *					ULONG	NumberOfPatches
 *					REPT	NumberOfPatches
 *							ULONG	Offset
 *							ULONG	Type
 *							ULONG	ExprSize
 *							UBYTE	Expr[ExprSize]
 *					ENDR
 *			ENDC
 *	ENDR
 */

	ULONG	totalsections;

	totalsections=fgetll(f);
	while(totalsections--)
	{
		SSection* section;

		section=sect_CreateNew();

		section->pGroups=groups;
		section->FileID=FileID;

		section->GroupID=fgetll(f);
		fgetasciiz(section->Name, MAXSYMNAMELENGTH, f);
		section->Bank=fgetll(f);
		section->Org=fgetll(f);

		section->TotalSymbols=read_symbols(f, &section->pSymbols);

		section->Size=fgetll(f);

		if((section->GroupID>=0) && (groups->Groups[section->GroupID].Type==GROUP_TEXT))
		{
			if((section->pData=malloc(section->Size))!=NULL)
			{
				fread(section->pData, 1, section->Size, f);
				section->pPatches=read_patches(f);
			}
		}
	}

	FileID+=1;
}

static	void	readchunk(FILE* f);

static	void	read_xob0(FILE* f)
{
	read_sections(read_groups(f), f);
}

static	void	read_xlb0(FILE* f)
{
	ULONG	count;

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
	ULONG	id;

	id=fgetll(f);

	switch(id)
	{
		case	MAKE_ID('X','O','B',0):
		{
			read_xob0(f);
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