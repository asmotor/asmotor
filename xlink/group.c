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
#include <stdint.h>
#include <string.h>

#include "mem.h"
#include "types.h"

#include "group.h"
#include "section.h"
#include "xlink.h"

#include <string.h>

#define HC800_MAX_BANKS 128

typedef struct MemoryChunk_ {
	struct MemoryChunk_* nextChunk;

	uint32_t cpuByteLocation;
	uint32_t size;
} MemoryChunk;

static MemoryGroup* s_machineGroups = NULL;

static MemoryGroup*
group_GetById(uint32_t groupId) {
	for (MemoryGroup* group = s_machineGroups; group != NULL; group = group->nextGroup) {
		if (group->groupId == groupId)
			return group;
	}

	return NULL;
}

static bool
group_GetFirstAndLastSection(MemoryGroup* group, SSection** begin, SSection** end) {
	// Find first section belonging to group
	SSection* first;
	for (first = sect_Sections; first != NULL; first = first->nextSection) {
		if (!first->used)
			continue;

		if (first->group != NULL && strcmp(first->group->name, group->name) == 0)
			break;
	}

	if (first == NULL)
		return NULL;

	// Find last section belonging to group
	SSection* last = first;
	SSection* candidate;
	for (candidate = last->nextSection; candidate != NULL; candidate = candidate->nextSection) {
		if (!candidate->used)
			continue;

		if (candidate->group != NULL && strcmp(candidate->group->name, group->name) == 0) {
			last = candidate;
		}
	}

	// Check that group does not appear in more sections
	for (candidate = last->nextSection; candidate != NULL; candidate = candidate->nextSection) {
		if (!candidate->used)
			continue;

		if (strcmp(candidate->group->name, group->name) == 0)
			return false;
	}

	// Group is contiguous, set min max
	*begin = first;
	*end = last;

	return true;
}

static bool
pool_AllocateMemory(MemoryPool* pool, uint32_t size, int32_t* cpuByteLocation) {
	for (MemoryChunk* chunk = pool->freeChunks; chunk != NULL; chunk = chunk->nextChunk) {
		if (chunk->size >= size) {
			*cpuByteLocation = chunk->cpuByteLocation;

			chunk->cpuByteLocation += size;
			chunk->size -= size;

			return true;
		}
	}

	return false;
}

static void
pool_CarveRange(MemoryChunk* chunk, uint32_t size, uint32_t cpuByteLocation) {
	if (cpuByteLocation == chunk->cpuByteLocation) {
		chunk->cpuByteLocation += size;
		chunk->size -= size;
	} else {
		MemoryChunk* newChunk = (MemoryChunk*) mem_Alloc(sizeof(MemoryChunk));

		newChunk->nextChunk = chunk->nextChunk;
		chunk->nextChunk = newChunk;

		newChunk->cpuByteLocation = cpuByteLocation + size;
		newChunk->size = chunk->cpuByteLocation + chunk->size - (cpuByteLocation + size);

		chunk->size = cpuByteLocation - chunk->cpuByteLocation;
	}
}

static bool
pool_AllocateAbsolute(MemoryPool* pool, uint32_t size, uint32_t cpuByteLocation) {
	for (MemoryChunk* chunk = pool->freeChunks; chunk != NULL; chunk = chunk->nextChunk) {
		if (cpuByteLocation >= chunk->cpuByteLocation && cpuByteLocation + size <= chunk->cpuByteLocation + chunk->size) {

			pool_CarveRange(chunk, size, cpuByteLocation);
			return true;
		}
	}

	return false;
}

static bool
pool_AllocateAligned(MemoryPool* pool, uint32_t size, uint32_t byteAlign, uint32_t pageSize, int32_t* resultLocation) {
	if (byteAlign == UINT32_MAX && pageSize == UINT32_MAX) {
		return pool_AllocateMemory(pool, size, resultLocation);
	}

	for (MemoryChunk* chunk = pool->freeChunks; chunk != NULL; chunk = chunk->nextChunk) {
		if (pageSize != UINT32_MAX) {
			// The requested memory is page aligned and possible byte aligned
			if (size > pageSize) {
				return false;
			}

			uint32_t page = chunk->cpuByteLocation;

			while (page < chunk->cpuByteLocation + chunk->size) {
				uint32_t remainingChunkSize = chunk->cpuByteLocation + chunk->size - page;
				uint32_t remainingPageSize = pageSize - page % pageSize;
				if (remainingPageSize > remainingChunkSize) {
					remainingPageSize = remainingChunkSize;
				}

				if (size <= remainingPageSize) {
					if (byteAlign != UINT32_MAX) {
						uint32_t cpuByteLocation = page + byteAlign - 1;
						cpuByteLocation -= cpuByteLocation % byteAlign;

						if (cpuByteLocation >= chunk->cpuByteLocation &&
						    cpuByteLocation + size <= chunk->cpuByteLocation + chunk->size) {

							pool_CarveRange(chunk, size, cpuByteLocation);

							*resultLocation = cpuByteLocation;
							return true;
						}
					} else {
						pool_CarveRange(chunk, size, page);

						*resultLocation = page;
						return true;
					}
				}
				page += remainingPageSize;
			}

			return false;
		}

		// Requested memory is byte aligned
		uint32_t cpuByteLocation = chunk->cpuByteLocation + byteAlign - 1;
		cpuByteLocation -= cpuByteLocation % byteAlign;
		if (cpuByteLocation >= chunk->cpuByteLocation && cpuByteLocation + size <= chunk->cpuByteLocation + chunk->size) {

			pool_CarveRange(chunk, size, cpuByteLocation);

			*resultLocation = cpuByteLocation;
			return true;
		}
	}

	return false;
}

static void
group_InitCommonGameboy(void) {
	MemoryGroup* group;

	//	Create VRAM group

	group = group_Create("VRAM", 1);
	group->pools[0] = pool_Create(-1, UINT32_MAX, 0x8000, 0, 0x2000, false);

	//	Create BSS group

	group = group_Create("BSS", 1);
	group->pools[0] = pool_Create(-1, UINT32_MAX, 0xC000, 0, 0x2000, false);

	//	Create HRAM group

	group = group_Create("HRAM", 1);
	group->pools[0] = pool_Create(-1, UINT32_MAX, 0xFF80, 0, 0x7F, false);
}

static MemoryGroup*
group_FindByName(const char* name) {
	for (MemoryGroup* group = s_machineGroups; group != NULL; group = group->nextGroup) {
		if (strcmp(group->name, name) == 0)
			return group;
	}

	error("Group \"%s\" undefined", name);
	return NULL;
}

static bool
group_AllocateAbsoluteFromGroup(MemoryGroup* group, uint32_t size, int32_t bankId, int32_t cpuByteLocation, int32_t* cpuBank,
                                int32_t* imageLocation, uint32_t* overlay) {
	for (int32_t i = 0; i < group->totalPools; ++i) {
		MemoryPool* pool = group->pools[i];

		if (bankId == -1 || bankId == pool->cpuBank) {
			if (pool_AllocateAbsolute(pool, size, cpuByteLocation)) {
				*cpuBank = pool->cpuBank;
				*imageLocation =
				    pool->imageLocation == -1 ? -1 : pool->imageLocation + cpuByteLocation - (int32_t) pool->cpuByteLocation;
				*overlay = pool->overlay;
				return true;
			}
		}
	}
	return false;
}

static bool
group_AllocateAlignedFromGroup(MemoryGroup* group, uint32_t size, int32_t bankId, int32_t byteAlign, int32_t pageSize,
                               int32_t* cpuByteLocation, int32_t* cpuBank, int32_t* imageLocation, uint32_t* overlay) {
	for (int32_t i = 0; i < group->totalPools; ++i) {
		MemoryPool* pool = group->pools[i];

		if (bankId == -1 || bankId == pool->cpuBank) {
			if (pool_AllocateAligned(pool, size, byteAlign, pageSize, cpuByteLocation)) {
				*cpuBank = pool->cpuBank;
				*imageLocation =
				    pool->imageLocation == -1 ? -1 : pool->imageLocation + *cpuByteLocation - (int32_t) pool->cpuByteLocation;
				*overlay = pool->overlay;
				return true;
			}
		}
	}
	return false;
}

extern void
group_InitMemoryChunks(void) {
	for (MemoryGroup* group = s_machineGroups; group != NULL; group = group->nextGroup) {
		for (int32_t i = 0; i < group->totalPools; ++i) {
			MemoryPool* pool = group->pools[i];

			if ((pool->freeChunks = (MemoryChunk*) mem_Alloc(sizeof(MemoryChunk))) != NULL) {
				pool->freeChunks->cpuByteLocation = pool->cpuByteLocation;
				pool->freeChunks->size = pool->size;
				pool->freeChunks->nextChunk = NULL;
			}
		}
	}
}

extern MemoryGroup*
group_Create(const char* groupName, uint32_t totalBanks) {
	uint32_t nextId = 0;

	MemoryGroup** ppgroup = &s_machineGroups;
	while (*ppgroup) {
		ppgroup = &(*ppgroup)->nextGroup;
		++nextId;
	}

	*ppgroup = (MemoryGroup*) mem_Alloc(sizeof(MemoryGroup) + sizeof(MemoryPool*) * totalBanks);

	strncpy((*ppgroup)->name, groupName, sizeof((*ppgroup)->name) - 1);
	(*ppgroup)->groupId = nextId;
	(*ppgroup)->nextGroup = NULL;
	(*ppgroup)->totalPools = totalBanks;

	return *ppgroup;
}

extern MemoryGroup*
group_Find(const char* groupName) {
	for (MemoryGroup* group = s_machineGroups; group != NULL; group = group->nextGroup) {
		if (strcmp(group->name, groupName) == 0) {
			return group;
		}
	}

	return NULL;
}

extern void
group_GetProperty(uint32_t groupId, ESymbolProperty property, SSection** section, int32_t* value) {
	MemoryGroup* group = group_GetById(groupId);
	if (group == NULL)
		error("Internal error - group ID not found");

	SSection* start;
	SSection* end;

	if (!group_GetFirstAndLastSection(group, &start, &end)) {
		error("Group \"%s\" is not contiguous", group->name);
	}

	switch (property) {
		case PROP_START: {
			*section = start;
			*value = start->cpuLocation != -1 ? start->cpuLocation : 0;
			break;
		}
		case PROP_SIZE: {
			*section = NULL;
			*value = (end->cpuLocation != -1 ? end->cpuLocation + end->size : end->size) -
			         (start->cpuLocation != -1 ? start->cpuLocation : 0);
			break;
		}
		default:
			error("Internal error - invalid property ID");
	}
}

extern MemoryPool*
pool_Create(int32_t imageLocation, uint32_t overlay, uint32_t cpuByteLocation, int32_t cpuBank, uint32_t size, bool onlyAbs) {
	MemoryPool* pool = (MemoryPool*) mem_Alloc(sizeof(MemoryPool));

	pool->imageLocation = imageLocation;
	pool->overlay = overlay;
	pool->cpuByteLocation = cpuByteLocation;
	pool->cpuBank = cpuBank;
	pool->size = size;
	pool->onlyAbs = onlyAbs;

	pool->freeChunks = NULL;

	return pool;
}

extern void
pool_Free(MemoryPool* pool) {
	MemoryChunk* chunk = pool->freeChunks;
	while (chunk != NULL) {
		MemoryChunk* next = chunk->nextChunk;
		mem_Free(chunk);
		chunk = next;
	}

	mem_Free(pool);
}

bool
group_AllocateAbsolute(const char* groupName, uint32_t size, int32_t bankId, int32_t cpuByteLocation, int32_t* cpuBank,
                       int32_t* imageLocation, uint32_t* overlay) {
	MemoryGroup* group = group_FindByName(groupName);
	return group_AllocateAbsoluteFromGroup(group, size, bankId, cpuByteLocation, cpuBank, imageLocation, overlay);
}

bool
group_AllocateAligned(const char* groupName, uint32_t size, int32_t bankId, int32_t byteAlign, int32_t pageSize,
                      int32_t* cpuByteLocation, int32_t* cpuBank, int32_t* imageLocation, uint32_t* overlay) {
	MemoryGroup* group = group_FindByName(groupName);
	return group_AllocateAlignedFromGroup(group, size, bankId, byteAlign, pageSize, cpuByteLocation, cpuBank, imageLocation,
	                                      overlay);
}

void
group_SetupGameboy(void) {
	MemoryPool* codepools[256];
	MemoryGroup* group;

	for (int i = 0; i < 256; ++i)
		codepools[i] = pool_Create(i * 0x4000, UINT32_MAX, i == 0 ? 0x0000 : 0x4000, i, 0x4000, false);

	//	Create HOME group

	group = group_Create("HOME", 1);
	group->pools[0] = codepools[0];

	//	Create CODE group

	group = group_Create("CODE", 256);
	for (int i = 0; i < 256; ++i)
		group->pools[i] = codepools[i];

	//	Create DATA group

	group = group_Create("DATA", 256);
	for (int i = 0; i < 256; ++i)
		group->pools[i] = codepools[i];

	// Create VRAM, BSS and HRAM

	group_InitCommonGameboy();
}

void
group_SetupSmallGameboy(void) {
	MemoryPool* codepool;
	MemoryGroup* group;

	codepool = pool_Create(0, UINT32_MAX, 0x0000, 0, 0x8000, false);

	//	Create HOME group

	group = group_Create("HOME", 1);
	group->pools[0] = codepool;

	//	Create CODE group

	group = group_Create("CODE", 1);
	group->pools[0] = codepool;

	//	Create DATA group

	group = group_Create("DATA", 1);
	group->pools[0] = codepool;

	// Create VRAM, BSS and HRAM

	group_InitCommonGameboy();
}

void
group_SetupAmiga(void) {
	MemoryGroup* group;
	MemoryPool* codepool;
	MemoryPool* chippool;

	codepool = pool_Create(0x00200000, UINT32_MAX, 0, 0, 0x3FFFFFFF, false);
	chippool = pool_Create(0x00000000, UINT32_MAX, 0, 0, 0x001FFFFF, false);

	//	Create CODE group
	group = group_Create("CODE", 1);
	group->pools[0] = codepool;

	//	Create CODE_C group
	group = group_Create("CODE_C", 1);
	group->pools[0] = chippool;

	//	Create DATA group
	group = group_Create("DATA", 1);
	group->pools[0] = codepool;

	//	Create DATA_C group
	group = group_Create("DATA_C", 1);
	group->pools[0] = chippool;

	//	Create BSS group
	group = group_Create("BSS", 1);
	group->pools[0] = pool_Create(-1, UINT32_MAX, 0, 0, 0x3FFFFFFF, false);

	//	Create BSS_C group
	group = group_Create("BSS_C", 1);
	group->pools[0] = pool_Create(-1, UINT32_MAX, 0, 0, 0x3FFFFFFF, false);
}

void
group_SetupSegaMegaDrive(void) {
	MemoryGroup* group;
	MemoryPool* rompool = pool_Create(0, UINT32_MAX, 0x000000, 0, 0x400000, false);
	MemoryPool* rampool = pool_Create(0, UINT32_MAX, 0xFF0000, 0, 0x10000, false);

	//	Create CODE group
	group = group_Create("CODE", 1);
	group->pools[0] = rompool;

	//	Create DATA group
	group = group_Create("DATA", 1);
	group->pools[0] = rompool;

	//	Create BSS group
	group = group_Create("BSS", 1);
	group->pools[0] = rampool;
}

void
group_SetupSegaMasterSystem(int size) {
	MemoryGroup* group;

	MemoryPool* homepool = pool_Create(0, UINT32_MAX, 0, 0, 0x400, false);
	MemoryPool* codepool = pool_Create(0x400, UINT32_MAX, 0x400, 0, size - 16 - 0x400, false);

	//	Create CODE group

	group = group_Create("CODE", 2);
	group->pools[0] = homepool;
	group->pools[1] = codepool;

	//	Create HOME group

	group = group_Create("HOME", 1);
	group->pools[0] = homepool;

	//	Create DATA group

	group = group_Create("DATA", 2);
	group->pools[0] = homepool;
	group->pools[1] = codepool;

	//	Create BSS group

	group = group_Create("BSS", 1);
	group->pools[0] = pool_Create(-1, UINT32_MAX, 0xC000, 0, 0x1FF8, false);
}

void
group_SetupSegaMasterSystemBanked(void) {
	MemoryPool* homepool;
	MemoryPool* codepools[65];
	MemoryGroup* group;

	homepool = pool_Create(0, UINT32_MAX, 0, 0, 0x400, false);

	codepools[0] = pool_Create(0x400, UINT32_MAX, 0x400, 0, 0x4000 - 0x400, false);
	codepools[1] = pool_Create(0x4000, UINT32_MAX, 0x4000, 1, 0x3FF0, false);
	for (int i = 2; i <= 63; ++i)
		codepools[i] = pool_Create(i * 0x4000, UINT32_MAX, 0x8000, i, 0x4000, false);

	//	Create HOME group

	group = group_Create("HOME", 1);
	group->pools[0] = homepool;

	//	Create CODE group

	group = group_Create("CODE", 65);
	group->pools[0] = homepool;
	for (int i = 1; i <= 64; ++i)
		group->pools[i] = codepools[i - 1];

	//	Create DATA group

	group = group_Create("DATA", 65);
	group->pools[0] = homepool;
	for (int i = 1; i <= 64; ++i)
		group->pools[i] = codepools[i - 1];

	//	Create BSS group

	group = group_Create("BSS", 1);
	group->pools[0] = pool_Create(-1, UINT32_MAX, 0xC000, 0, 0x1FF8, false);
}

void
group_SetupHC8XXROM(void) {
	MemoryGroup* group;

	MemoryPool* codepool = pool_Create(0, UINT32_MAX, 0, 1, 0x4000, false);
	MemoryPool* bss = pool_Create(-1, UINT32_MAX, 0x0000, 0x80, 0x4000, false);

	//	Create HOME group

	group = group_Create("HOME", 1);
	group->pools[0] = codepool;

	//	Create CODE group

	group = group_Create("CODE", 1);
	group->pools[0] = codepool;

	//	Create DATA group

	group = group_Create("DATA", 1);
	group->pools[0] = codepool;

	//	Create BSS group

	group = group_Create("BSS", 1);
	group->pools[0] = bss;

	/* Kernel should not use shared sections, it must not put CODE or BSS there */
#if 0

    //	Create CODE_S (shared) group

    group = group_Create("CODE_S", 1);
    group->pools[0] = codepool;

    //	Create DATA_S (shared) group

    group = group_Create("DATA_S", 1);
    group->pools[0] = codepool;

    //	Create BSS_S (shared) group

    group = group_Create("BSS_S", 1);
    group->pools[0] = bss;

#endif
}

void
group_SetupHC8XXSmall(void) {
	/* HC800, CODE: 64 KiB text + data + bss */
	MemoryGroup* group;

	MemoryPool* home = pool_Create(0, UINT32_MAX, 0, 0x81, 0x4000, false);
	MemoryPool* shared = pool_Create(0, UINT32_MAX, 0x4000, 0x82, 0xC000, false);

	//	Create HOME group

	group = group_Create("HOME", 1);
	group->pools[0] = home;

	//	Create CODE group

	group = group_Create("CODE", 2);
	group->pools[0] = home;
	group->pools[1] = shared;

	//	Create DATA group

	group = group_Create("DATA", 2);
	group->pools[0] = home;
	group->pools[1] = shared;

	//	Create BSS group

	group = group_Create("BSS", 2);
	group->pools[0] = home;
	group->pools[1] = shared;

	//	Create CODE_S group

	group = group_Create("CODE_S", 1);
	group->pools[0] = shared;

	//	Create DATA_S group

	group = group_Create("DATA_S", 1);
	group->pools[0] = shared;

	//	Create BSS_S group

	group = group_Create("BSS_S", 1);
	group->pools[0] = shared;
}

void
group_SetupHC8XXSmallHarvard(void) {
	/* HC800 CODE: 64 KiB text, DATA: 64 KiB data + bss */
	MemoryGroup* group;

	MemoryPool* home = pool_Create(0, UINT32_MAX, 0, 0x81, 0x4000, false);
	MemoryPool* codeShared = pool_Create(0, UINT32_MAX, 0x4000, 0x82, 0xC000, false);
	MemoryPool* data = pool_Create(0, UINT32_MAX, 0, 0x85, 0x4000, false);
	MemoryPool* dataShared = pool_Create(0, UINT32_MAX, 0x4000, 0x86, 0xC000, false);

	//	Create HOME group

	group = group_Create("HOME", 1);
	group->pools[0] = home;

	//	Create CODE group

	group = group_Create("CODE", 2);
	group->pools[0] = home;
	group->pools[1] = codeShared;

	//	Create DATA group

	group = group_Create("DATA", 2);
	group->pools[0] = data;
	group->pools[1] = dataShared;

	//	Create BSS group

	group = group_Create("BSS", 2);
	group->pools[0] = data;
	group->pools[1] = dataShared;

	//	Create CODE_S group

	group = group_Create("CODE_S", 1);
	group->pools[0] = codeShared;

	//	Create DATA_S group

	group = group_Create("DATA_S", 1);
	group->pools[0] = dataShared;

	//	Create BSS_S (home) group

	group = group_Create("BSS_S", 1);
	group->pools[0] = dataShared;
}

void
group_SetupHC8XXMedium(void) {
	/* HC800, CODE: 32 KiB text + data + bss, CODE: 32 KiB sized banks text */
	MemoryGroup* group;

	MemoryPool* home = pool_Create(0, UINT32_MAX, 0, 0x81, 0x4000, false);
	MemoryPool* shared = pool_Create(0, UINT32_MAX, 0x4000, 0x82, 0x8000, false);
	int firstCodeBank = 0x83;
	int codeBanks = (0x100 - firstCodeBank) / 2;
	MemoryPool* banks[HC800_MAX_BANKS];
	for (int i = 0; i < codeBanks; ++i) {
		banks[i] = pool_Create(0, UINT32_MAX, 0x8000, i * 2 + firstCodeBank, 0x8000, false);
	}

	//	Create HOME group

	group = group_Create("HOME", 2);
	group->pools[0] = home;
	group->pools[1] = shared;

	//	Create CODE group

	group = group_Create("CODE", 2 + codeBanks);
	group->pools[0] = home;
	group->pools[1] = shared;
	for (int i = 0; i < codeBanks; ++i) {
		group->pools[i + 2] = banks[i];
	}

	//	Create DATA group

	group = group_Create("DATA", 2);
	group->pools[0] = home;
	group->pools[1] = shared;

	//	Create BSS group

	group = group_Create("BSS", 2);
	group->pools[0] = home;
	group->pools[1] = shared;

	//	Create CODE_S (shared) group

	group = group_Create("CODE_S", 1);
	group->pools[0] = shared;

	//	Create DATA_S (shared) group

	group = group_Create("DATA_S", 1);
	group->pools[0] = shared;

	//	Create BSS_S (shared) group

	group = group_Create("BSS_S", 1);
	group->pools[0] = shared;
}

void
group_SetupHC8XXMediumHarvard(void) {
	/* HC800, CODE: 32 KiB text, CODE: 32 KiB sized text banks, DATA: 64 KiB data + bss */
	MemoryGroup* group;

	MemoryPool* home = pool_Create(0, UINT32_MAX, 0, 0x81, 0x4000, false);
	MemoryPool* shared = pool_Create(0, UINT32_MAX, 0x4000, 0x82, 0x4000, false);
	MemoryPool* data = pool_Create(0, 0, UINT32_MAX, 0x83, 0x4000, false);
	MemoryPool* dataShared = pool_Create(0, UINT32_MAX, 0x4000, 0x84, 0xC000, false);

	int firstCodeBank = 0x87;
	int codeBanks = (0x100 - firstCodeBank) / 2;
	MemoryPool* banks[HC800_MAX_BANKS];
	for (int i = 0; i < codeBanks; ++i) {
		banks[i] = pool_Create(0, UINT32_MAX, 0x8000, i * 2 + firstCodeBank, 0x8000, false);
	}

	//	Create HOME group

	group = group_Create("HOME", 2);
	group->pools[0] = home;
	group->pools[1] = shared;

	//	Create CODE group

	group = group_Create("CODE", 2 + codeBanks);
	group->pools[0] = home;
	group->pools[1] = shared;
	for (int i = 0; i < codeBanks; ++i) {
		group->pools[i + 2] = banks[i];
	}

	//	Create DATA group

	group = group_Create("DATA", 2);
	group->pools[0] = data;
	group->pools[1] = dataShared;

	//	Create BSS group

	group = group_Create("BSS", 2);
	group->pools[0] = data;
	group->pools[1] = dataShared;

	//	Create CODE_S (shared) group

	group = group_Create("CODE_S", 1);
	group->pools[0] = shared;

	//	Create DATA_S (shared) group

	group = group_Create("DATA_S", 1);
	group->pools[0] = dataShared;

	//	Create BSS_S (shared) group

	group = group_Create("BSS_S", 1);
	group->pools[0] = dataShared;
}

void
group_SetupHC8XXLarge(void) {
	/* HC800, CODE: 32 KiB text + data + bss, CODE: 32 KiB sized banks text + data + bss */

	MemoryGroup* group;

	MemoryPool* home = pool_Create(0, UINT32_MAX, 0, 0x81, 0x4000, false);
	MemoryPool* shared = pool_Create(0, UINT32_MAX, 0x4000, 0x82, 0x4000, false);
	int firstCodeBank = 0x83;
	int codeBanks = (0x100 - firstCodeBank) / 2;
	MemoryPool* banks[HC800_MAX_BANKS];
	for (int i = 0; i < codeBanks; ++i) {
		banks[i] = pool_Create(0, UINT32_MAX, 0x8000, i * 2 + firstCodeBank, 0x8000, false);
	}

	//	Create HOME group

	group = group_Create("HOME", 2);
	group->pools[0] = home;
	group->pools[1] = shared;

	//	Create CODE group

	group = group_Create("CODE", 2 + codeBanks);
	group->pools[0] = home;
	group->pools[1] = shared;
	for (int i = 0; i < codeBanks; ++i) {
		group->pools[i + 2] = banks[i];
	}

	//	Create DATA group

	group = group_Create("DATA", 2 + codeBanks);
	group->pools[0] = home;
	group->pools[1] = shared;
	for (int i = 0; i < codeBanks; ++i) {
		group->pools[i + 2] = banks[i];
	}

	//	Create BSS group

	group = group_Create("BSS", 2 + codeBanks);
	group->pools[0] = home;
	group->pools[1] = shared;
	for (int i = 0; i < codeBanks; ++i) {
		group->pools[i + 2] = banks[i];
	}

	//	Create CODE_S (shared) group

	group = group_Create("CODE_S", 1);
	group->pools[0] = shared;

	//	Create DATA_S (shared) group

	group = group_Create("DATA_S", 1);
	group->pools[0] = shared;

	//	Create BSS_S (shared) group

	group = group_Create("BSS_S", 1);
	group->pools[0] = shared;
}

void
group_SetupHC8XXLargeHarvard(void) {
	/* HC800, CODE: 32 KiB text, CODE: 32 KiB sized text banks, DATA: 64 KiB data + bss */

	MemoryGroup* group;

	MemoryPool* home = pool_Create(0, UINT32_MAX, 0, 0x81, 0x4000, false);
	MemoryPool* sharedCode = pool_Create(0, UINT32_MAX, 0x4000, 0x82, 0x4000, false);
	MemoryPool* bssPool = pool_Create(0, UINT32_MAX, 0x0000, 0x83, 0x4000, false);
	MemoryPool* sharedBssPool = pool_Create(0, UINT32_MAX, 0x4000, 0x84, 0xC000, false);
	int firstCodeBank = 0x87;
	int codeBanks = (0x100 - firstCodeBank) / 2;

	//	Create HOME group

	group = group_Create("HOME", 2);
	group->pools[0] = home;
	group->pools[1] = sharedCode;

	//	Create CODE group

	group = group_Create("CODE", 2 + codeBanks);
	group->pools[0] = home;
	group->pools[1] = sharedCode;
	for (int i = 0; i < codeBanks; ++i) {
		group->pools[i + 2] = pool_Create(0, UINT32_MAX, 0x8000, firstCodeBank + i * 2, 0x8000, false);
	}

	//	Create DATA group

	group = group_Create("DATA", 2);
	group->pools[0] = bssPool;
	group->pools[1] = sharedBssPool;

	//	Create BSS group

	group = group_Create("BSS", 2);
	group->pools[0] = bssPool;
	group->pools[1] = sharedBssPool;

	//	Create CODE_S (shared) group

	group = group_Create("CODE_S", 1);
	group->pools[0] = sharedCode;

	//	Create DATA_S (shared) group

	group = group_Create("DATA_S", 1);
	group->pools[0] = sharedBssPool;

	//	Create BSS_S (shared) group

	group = group_Create("BSS_S", 1);
	group->pools[0] = sharedBssPool;
}

void
group_SetupCoCo(void) {
	MemoryGroup* group;

	MemoryPool* system_ram = pool_Create(0, UINT32_MAX, 0x600, 0, 0x8000 - 0x600, false);

	//	Create CODE group

	group = group_Create("CODE", 1);
	group->pools[0] = system_ram;

	//	Create DATA group

	group = group_Create("DATA", 1);
	group->pools[0] = system_ram;

	//	Create BSS group

	group = group_Create("BSS", 1);
	group->pools[0] = system_ram;
}

extern bool
group_NeedsOverlay(void) {
	for (MemoryGroup* group = s_machineGroups; group != NULL; group = group->nextGroup) {
		for (int32_t i = 0; i < group->totalPools; ++i) {
			if (group->pools[i]->overlay != UINT32_MAX)
				return true;
		}
	}

	return false;
}
