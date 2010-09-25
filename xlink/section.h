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

#ifndef	SECTION_H
#define	SECTION_H

typedef	struct	_SSection
{
	SGroups* 	pGroups;

	ULONG		FileID;

	SLONG		GroupID;

	//	Before assigned Bank and Org reflect the programmers wish.
	//	After, they point to where this section actually is
	SLONG		Bank;
	SLONG		Org;

	char		Name[MAXSYMNAMELENGTH];

	ULONG		TotalSymbols;
	SSymbol		*pSymbols;

	ULONG		Size;

	UBYTE		*pData;
	SPatches* pPatches;

	BOOL		Used;
	BOOL		Assigned;

	struct	_SSection* pNext;
}	SSection;

extern	SSection* pSections;

extern	SSection* sect_CreateNew(void);
extern	SLONG		sect_GetSymbolValue(SSection* sect, SLONG symbolid);
extern	SLONG		sect_GetSymbolBank(SSection* sect, SLONG symbolid);

#endif