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
	SGroups* pGroups;

	uint32_t	FileID;

	int32_t	GroupID;

	/* Before assigned Bank, Position and BasePC reflect the programmer's wish.
	 * After, they point to where this section actually is
	 */
	int32_t	Bank;
	int32_t	Position;
	int32_t	BasePC;
	int32_t	ImageOffset;
	int32_t MinimumWordSize;

	char	Name[MAXSYMNAMELENGTH];

	uint32_t	TotalSymbols;
	SSymbol* pSymbols;

	uint32_t	Size;

	uint8_t*	pData;
	SPatches*	pPatches;

	bool_t	Used;
	bool_t	Assigned;

	struct _SSection* pNext;
}	SSection;

extern SSection* pSections;

extern SSection* sect_CreateNew(void);
extern int32_t sect_GetSymbolValue(SSection* sect, int32_t symbolid);
extern int32_t sect_GetSymbolBank(SSection* sect, int32_t symbolid);
extern char* sect_GetSymbolName(SSection* sect, int32_t symbolid);

#endif
