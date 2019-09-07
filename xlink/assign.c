/*  Copyright 2008-2017 Carsten Elton Sorensen

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

#include "section.h"
#include "xlink.h"

static void
assignOrgAndBankFixedSection(SSection* section, intptr_t data) {
    if (!section->assigned && !sect_IsEquSection(section) && section->cpuByteLocation != -1 && section->cpuBank != -1) {
        if (!group_AllocateAbsolute(section->group->name, section->size, section->cpuBank, section->cpuByteLocation,
                                    &section->cpuBank, &section->imageLocation))
            error("No space for section \"%s\"", section->name);

        section->assigned = true;
    }
}

static void
assignOrgFixedSection(SSection* section, intptr_t data) {
    if (!section->assigned && !sect_IsEquSection(section) && section->cpuByteLocation != -1 && section->cpuBank == -1) {
        if (!group_AllocateAbsolute(section->group->name, section->size, section->cpuBank, section->cpuByteLocation,
                                    &section->cpuBank, &section->imageLocation))
            error("No space for section \"%s\"", section->name);

        section->assigned = true;
    }
}

static void
assignBankFixedSection(SSection* section, intptr_t data) {
    if (!section->assigned && !sect_IsEquSection(section) && section->cpuByteLocation == -1 && section->cpuBank != -1) {
        if (!group_AllocateMemory(section->group->name, section->size, section->cpuBank, &section->cpuByteLocation,
                                  &section->cpuBank, &section->imageLocation))
            error("No space for section \"%s\"", section->name);

        section->cpuLocation = section->cpuByteLocation / section->minimumWordSize;
        section->assigned = true;
    }
}

static void
assignTextSection(SSection* section, intptr_t data) {
    if (!section->assigned && !sect_IsEquSection(section) && section->group->type == GROUP_TEXT) {
        if (!group_AllocateMemory(section->group->name, section->size, section->cpuBank, &section->cpuByteLocation,
                                  &section->cpuBank, &section->imageLocation))
            error("No space for section \"%s\"", section->name);

        section->cpuLocation = section->cpuByteLocation / section->minimumWordSize;
        section->assigned = true;
    }
}

static void
assignSection(SSection* section, intptr_t data) {
    if (sect_IsEquSection(section)) {
        //	This is a special exported EQU symbol section

        section->cpuByteLocation = 0;
        section->cpuLocation = 0;
        section->cpuBank = 0;
        section->imageLocation = -1;
        section->assigned = true;
    } else if (!section->assigned) {
        if (!group_AllocateMemory(section->group->name, section->size, section->cpuBank, &section->cpuByteLocation,
                                  &section->cpuBank, &section->imageLocation))
            error("No space for section \"%s\"", section->name);

        section->cpuLocation = section->cpuByteLocation / section->minimumWordSize;
        section->assigned = true;
    }
}

void
assign_Process(void) {
    sect_ForEachUsedSection(assignOrgAndBankFixedSection, 0);
    sect_ForEachUsedSection(assignOrgFixedSection, 0);
    sect_ForEachUsedSection(assignBankFixedSection, 0);
    // Byte aligned sections should go here
    sect_ForEachUsedSection(assignTextSection, 0);
    sect_ForEachUsedSection(assignSection, 0);

    sect_SortSections();
}
