/*  Copyright 2008-2026 Carsten Elton Sorensen and contributors

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

#include <stdbool.h>
#include <stdlib.h>

#include "group.h"
#include "object.h"
#include "section.h"
#include "xlink.h"

typedef bool (*sectionPredicate)(SSection*);

static void
assignSectionCore(SSection* section, intptr_t data) {
	sectionPredicate predicate = (sectionPredicate) data;
	if (predicate(section)) {
		if (!group_AllocateAligned(section->group->name, section->size, section->cpuBank, section->byteAlign, section->page,
		                           &section->cpuByteLocation, &section->cpuBank, &section->imageLocation, &section->overlay))
			error("No space for section \"%s\"", section->name);

		section->cpuLocation = section->cpuByteLocation / section->minimumWordSize;
		section->assigned = true;
	}
}

static void
assignOrgAndBankFixedSection(SSection* section, intptr_t data) {
	sectionPredicate predicate = (sectionPredicate) data;
	if (predicate(section) && section->cpuByteLocation != -1 && section->cpuBank != -1) {
		if (!group_AllocateAbsolute(section->group->name, section->size, section->cpuBank, section->cpuByteLocation,
		                            &section->cpuBank, &section->imageLocation, &section->overlay))
			error("No space for section \"%s\"", section->name);

		section->assigned = true;
	}
}

static void
assignOrgFixedSection(SSection* section, intptr_t data) {
	sectionPredicate predicate = (sectionPredicate) data;
	if (predicate(section) && section->cpuByteLocation != -1 && section->cpuBank == -1) {
		if (!group_AllocateAbsolute(section->group->name, section->size, section->cpuBank, section->cpuByteLocation,
		                            &section->cpuBank, &section->imageLocation, &section->overlay))
			error("No space for section \"%s\"", section->name);

		section->assigned = true;
	}
}

static void
assignBankedAlignedPagedSection(SSection* section, intptr_t data) {
	if (section->cpuBank != -1 && section->byteAlign != -1 && section->page != -1) {
		assignSectionCore(section, data);
	}
}

static void
assignPagedSection(SSection* section, intptr_t data) {
	if (section->cpuBank == -1 && section->byteAlign == -1 && section->page != -1) {
		assignSectionCore(section, data);
	}
}

static void
assignAlignedSection(SSection* section, intptr_t data) {
	if (section->cpuBank == -1 && section->byteAlign != -1 && section->page == -1) {
		assignSectionCore(section, data);
	}
}

static void
assignAlignedPagedSection(SSection* section, intptr_t data) {
	if (section->cpuBank == -1 && section->byteAlign != -1 && section->page != -1) {
		assignSectionCore(section, data);
	}
}

static void
assignBankedPagedSection(SSection* section, intptr_t data) {
	if (section->cpuBank != -1 && section->byteAlign == -1 && section->page != -1) {
		assignSectionCore(section, data);
	}
}

static void
assignBankedAlignedSection(SSection* section, intptr_t data) {
	if (section->cpuBank != -1 && section->byteAlign != -1 && section->page == -1) {
		assignSectionCore(section, data);
	}
}

static void
assignSection(SSection* section, intptr_t data) {
	if (!section->assigned) {
		sectionPredicate predicate = (sectionPredicate) data;

		if (sect_IsEquSection(section)) {
			//	This is a special exported EQU symbol section

			section->cpuByteLocation = 0;
			section->cpuLocation = 0;
			section->cpuBank = 0;
			section->imageLocation = -1;
			section->assigned = true;
		} else if (predicate(section)) {
			if (!group_AllocateAligned(section->group->name, section->size, section->cpuBank, section->byteAlign, section->page,
			                           &section->cpuByteLocation, &section->cpuBank, &section->imageLocation, &section->overlay))
				error("No space for section \"%s\"", section->name);

			section->cpuLocation = section->cpuByteLocation / section->minimumWordSize;
			section->assigned = true;
		}
	}
}

static bool
isCode(SSection* section) {
	return !section->assigned && section->group != NULL && section->group->type == GROUP_TEXT && section->group->flags == 0;
}

static bool
isCodeShared(SSection* section) {
	return !section->assigned && section->group != NULL && section->group->type == GROUP_TEXT &&
	       section->group->flags == GROUP_FLAG_SHARED;
}

static bool
isData(SSection* section) {
	return !section->assigned && section->group != NULL && section->group->type == GROUP_TEXT &&
	       section->group->flags == GROUP_FLAG_DATA;
}

static bool
isDataShared(SSection* section) {
	return !section->assigned && section->group != NULL && section->group->type == GROUP_TEXT &&
	       section->group->flags == (GROUP_FLAG_DATA | GROUP_FLAG_SHARED);
}

static bool
isBSS(SSection* section) {
	return !section->assigned && section->group != NULL && section->group->type == GROUP_BSS && section->group->flags == 0;
}

static bool
isBSSShared(SSection* section) {
	return !section->assigned && section->group != NULL && section->group->type == GROUP_BSS &&
	       section->group->flags == GROUP_FLAG_SHARED;
}

static bool
truePredicate(SSection* section) {
	return true;
}

#define TOTAL_PREDICATES 6
static sectionPredicate sectionPredicates[TOTAL_PREDICATES] = {
    isCodeShared, isCode, isDataShared, isData, isBSSShared, isBSS,
};

void
assign_Process(void) {
	for (int i = 0; i < TOTAL_PREDICATES; ++i) {
		intptr_t pred = (intptr_t) sectionPredicates[i];

		sect_ForEachUsedSection(assignOrgAndBankFixedSection, pred);
		sect_ForEachUsedSection(assignOrgFixedSection, pred);
		sect_ForEachUsedSection(assignBankedAlignedPagedSection, pred);
		sect_ForEachUsedSection(assignBankedAlignedSection, pred);
		sect_ForEachUsedSection(assignBankedPagedSection, pred);
		sect_ForEachUsedSection(assignAlignedPagedSection, pred);
		sect_ForEachUsedSection(assignAlignedSection, pred);
		sect_ForEachUsedSection(assignPagedSection, pred);
		sect_ForEachUsedSection(assignSection, pred);
	}
	sect_ForEachUsedSection(assignSection, (intptr_t) truePredicate);

	sect_SortSections();
}
