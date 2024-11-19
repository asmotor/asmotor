/*  Copyright 2008-2022 Carsten Elton Sorensen and contributors

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

#ifndef XLINK_OBJECT_H_INCLUDED_
#define XLINK_OBJECT_H_INCLUDED_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "str.h"

#include "symbol.h"

typedef enum {
    GROUP_TEXT = 0,
    GROUP_BSS = 1
} GroupType;

#define GROUP_FLAG_SHARED 0x20000000u
#define GROUP_FLAG_DATA   0x40000000u

typedef struct {
    char name[MAX_SYMBOL_NAME_LENGTH];
    GroupType type;
    uint32_t flags;
} Group;

typedef struct {
    uint32_t totalGroups;
    Group groups[];
} Groups;

static inline bool
group_isText(Group* group) {
    return group != NULL && group->type == GROUP_TEXT;
}

static inline char*
group_Name(Group* group) {
    return group != NULL ? group->name : NULL;
}

static inline Group*
groups_GetGroup(Groups* groups, uint32_t groupId) {
    return groupId != UINT32_MAX && groupId < groups->totalGroups ? &groups->groups[groupId] : NULL;
}

extern const string*
obj_GetFilename(uint32_t fileInfoIndex);

extern void
obj_Read(char* fileName);

#endif
