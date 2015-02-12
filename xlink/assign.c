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

#include "xlink.h"


static void assignOrgAndBankFixedSection(Section* section)
{
	if (!section->assigned && section->group != NULL && section->cpuByteLocation != -1 && section->cpuBank != -1)
	{
		if (!group_AllocateAbsolute(section->group->name, section->size, section->cpuBank, section->cpuByteLocation, &section->cpuBank, &section->imageLocation))
			Error("No space for section \"%s\"", section->name);

		section->assigned = true;
	}
}


static void assignOrgFixedSection(Section* section)
{
	if (!section->assigned && section->group != NULL && section->cpuByteLocation != -1 && section->cpuBank == -1)
	{
		if (!group_AllocateAbsolute(section->group->name, section->size, section->cpuBank, section->cpuByteLocation, &section->cpuBank, &section->imageLocation))
			Error("No space for section \"%s\"", section->name);

		section->assigned = true;
	}
}


static void assignBankFixedSection(Section* section)
{
	if (!section->assigned && section->group != NULL && section->cpuByteLocation == -1 && section->cpuBank != -1)
	{
		if (!group_AllocateMemory(section->group->name, section->size, section->cpuBank, &section->cpuByteLocation, &section->cpuBank, &section->imageLocation))
			Error("No space for section \"%s\"", section->name);

		section->cpuLocation = section->cpuByteLocation / section->minimumWordSize;
		section->assigned = true;
	}
}


static void assignSection(Section* section)
{
	if (section->group == NULL)
	{
		//	This is a special exported EQU symbol section

		section->cpuByteLocation = 0;
		section->cpuLocation = 0;
		section->cpuBank = 0;
		section->imageLocation = -1;
		section->assigned = true;
	}
	else if (!section->assigned)
	{
		if (!group_AllocateMemory(section->group->name, section->size, section->cpuBank, &section->cpuByteLocation, &section->cpuBank, &section->imageLocation))
			Error("No space for section \"%s\"", section->name);

		section->cpuLocation = section->cpuByteLocation / section->minimumWordSize;
		section->assigned = true;
	}
}


void assign_Process(void)
{
	sect_ForEachUsedSection(assignOrgAndBankFixedSection);
	sect_ForEachUsedSection(assignOrgFixedSection);
	sect_ForEachUsedSection(assignBankFixedSection);
	// Byte aligned sections should go here
	sect_ForEachUsedSection(assignSection);
}
