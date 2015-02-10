/*  Copyright 2008-2015 Carsten Elton Sorensen

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

#ifndef	XLINK_SECTION_H_INCLUDED_
#define	XLINK_SECTION_H_INCLUDED_

typedef	struct Section_
{
	uint32_t fileId;
    uint32_t sectionId;

	Group* group;

	// Before they are assigned, bank, byteLocation and basePC reflect the programmer's wish.
	// After, they point to where this section actually is
	int32_t	cpuByteLocation; // Where the CPU sees this section, in bytes
	int32_t	cpuBank;
	int32_t	cpuLocation;
	int32_t	imageLocation;
	int32_t minimumWordSize;

	char name[MAXSYMNAMELENGTH];

	uint32_t totalSymbols;
	Symbol* symbols;

	uint32_t size;
	uint8_t* data;

	Patches* patches;

    struct Relocs_* relocs;

	bool_t used;
	bool_t assigned;

	struct Section_* nextSection;
} Section;

extern Section* g_sections;

extern Section* sect_CreateNew(void);
extern bool_t sect_GetConstantSymbolValue(Section* section, int32_t symbolId, int32_t* outValue);
extern bool_t sect_GetConstantSymbolBank(Section* section, int32_t symbolId, int32_t* outValue);
extern char* sect_GetSymbolName(Section* section, int32_t symbolId);

#endif
