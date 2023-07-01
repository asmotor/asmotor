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

#include "mem.h"
#include "str.h"
#include "types.h"

#include "group.h"
#include "symbol.h"
#include "xlink.h"

#include <string.h>

#define HC800_MAX_BANKS 128

typedef struct MemoryChunk_ {
    struct MemoryChunk_* nextChunk;

    uint32_t cpuByteLocation;
    uint32_t size;
} MemoryChunk;

static MemoryGroup* s_machineGroups = NULL;

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
        if (cpuByteLocation >= chunk->cpuByteLocation
            && cpuByteLocation + size <= chunk->cpuByteLocation + chunk->size) {

			pool_CarveRange(chunk, size, cpuByteLocation);
            return true;
        }
    }

    return false;
}

static bool
pool_AllocateAligned(MemoryPool* pool, uint32_t size, uint32_t byteAlign, int32_t* resultLocation) {
    for (MemoryChunk* chunk = pool->freeChunks; chunk != NULL; chunk = chunk->nextChunk) {
		uint32_t cpuByteLocation = chunk->cpuByteLocation + byteAlign - 1;
		cpuByteLocation -= cpuByteLocation % byteAlign;
        if (cpuByteLocation >= chunk->cpuByteLocation
            && cpuByteLocation + size <= chunk->cpuByteLocation + chunk->size) {

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
    group->pools[0] = pool_Create(-1, 0x8000, 0, 0x2000);

    //	Create BSS group

    group = group_Create("BSS", 1);
    group->pools[0] = pool_Create(-1, 0xC000, 0, 0x2000);

    //	Create HRAM group

    group = group_Create("HRAM", 1);
    group->pools[0] = pool_Create(-1, 0xFF80, 0, 0x7F);
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
group_AllocateMemoryFromGroup(MemoryGroup* group, uint32_t size, int32_t bankId, int32_t* cpuByteLocation,
                              int32_t* cpuBank, int32_t* imageLocation) {
    for (int32_t i = 0; i < group->totalPools; ++i) {
        MemoryPool* pool = group->pools[i];

        if (bankId == -1 || bankId == pool->cpuBank) {
            if (pool_AllocateMemory(pool, size, cpuByteLocation)) {
                *cpuBank = pool->cpuBank;
                *imageLocation = pool->imageLocation == -1 ? -1 : pool->imageLocation + *cpuByteLocation - (int32_t) pool->cpuByteLocation;
                return true;
            }
        }
    }
    return false;
}

static bool
group_AllocateAbsoluteFromGroup(MemoryGroup* group, uint32_t size, int32_t bankId, int32_t cpuByteLocation,
                                int32_t* cpuBank, int32_t* imageLocation) {
    for (int32_t i = 0; i < group->totalPools; ++i) {
        MemoryPool* pool = group->pools[i];

        if (bankId == -1 || bankId == pool->cpuBank) {
            if (pool_AllocateAbsolute(pool, size, cpuByteLocation)) {
                *cpuBank = pool->cpuBank;
                *imageLocation = pool->imageLocation == -1 ? -1 : pool->imageLocation + cpuByteLocation - (int32_t) pool->cpuByteLocation;
                return true;
            }
        }
    }
    return false;
}

static bool
group_AllocateAlignedFromGroup(MemoryGroup* group, uint32_t size, int32_t bankId, int32_t byteAlign,
                               int32_t* cpuByteLocation, int32_t* cpuBank, int32_t* imageLocation) {
    for (int32_t i = 0; i < group->totalPools; ++i) {
        MemoryPool* pool = group->pools[i];

        if (bankId == -1 || bankId == pool->cpuBank) {
            if (pool_AllocateAligned(pool, size, byteAlign, cpuByteLocation)) {
                *cpuBank = pool->cpuBank;
                *imageLocation = pool->imageLocation == -1 ? -1 : pool->imageLocation + *cpuByteLocation - (int32_t) pool->cpuByteLocation;
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
    MemoryGroup** ppgroup = &s_machineGroups;
    while (*ppgroup)
        ppgroup = &(*ppgroup)->nextGroup;

    *ppgroup = (MemoryGroup*) mem_Alloc(sizeof(MemoryGroup) + sizeof(MemoryPool*) * totalBanks);

    strncpy((*ppgroup)->name, groupName, sizeof((*ppgroup)->name) - 1);
    (*ppgroup)->nextGroup = NULL;
    (*ppgroup)->totalPools = totalBanks;

    return *ppgroup;
}


extern MemoryPool*
pool_Create(int32_t imageLocation, uint32_t cpuByteLocation, int32_t cpuBank, uint32_t size) {
    MemoryPool* pool = (MemoryPool*) mem_Alloc(sizeof(MemoryPool));

    pool->imageLocation = imageLocation;
    pool->cpuByteLocation = cpuByteLocation;
    pool->cpuBank = cpuBank;
    pool->size = size;

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
group_AllocateMemory(const char* groupName, uint32_t size, int32_t bankId, int32_t* cpuByteLocation, int32_t* cpuBank,
                     int32_t* imageLocation) {
    MemoryGroup* group = group_FindByName(groupName);
    return group_AllocateMemoryFromGroup(group, size, bankId, cpuByteLocation, cpuBank, imageLocation);

}

bool
group_AllocateAbsolute(const char* groupName, uint32_t size, int32_t bankId, int32_t cpuByteLocation, int32_t* cpuBank,
                       int32_t* imageLocation) {
    MemoryGroup* group = group_FindByName(groupName);
    return group_AllocateAbsoluteFromGroup(group, size, bankId, cpuByteLocation, cpuBank, imageLocation);
}

bool
group_AllocateAligned(const char* groupName, uint32_t size, int32_t bankId, int32_t byteAlign, int32_t* cpuByteLocation,
                      int32_t* cpuBank, int32_t* imageLocation) {
    MemoryGroup* group = group_FindByName(groupName);
    return group_AllocateAlignedFromGroup(group, size, bankId, byteAlign, cpuByteLocation, cpuBank, imageLocation);
}

void
group_SetupGameboy(void) {
    MemoryPool* codepools[256];
    MemoryGroup* group;

    for (int i = 0; i < 256; ++i)
        codepools[i] = pool_Create(i * 0x4000, i == 0 ? 0x0000 : 0x4000, i, 0x4000);

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

    codepool = pool_Create(0, 0x0000, 0, 0x8000);

    //	Create HOME group

    group = group_Create("HOME", 1);
    group->pools[0] = codepool;

    //	Create CODE group

    group = group_Create("CODE", 256);
    for (int i = 0; i < 256; ++i)
        group->pools[i] = codepool;

    //	Create DATA group

    group = group_Create("DATA", 256);
    for (int i = 0; i < 256; ++i)
        group->pools[i] = codepool;

    // Create VRAM, BSS and HRAM

    group_InitCommonGameboy();
}

void
group_SetupAmiga(void) {
    MemoryGroup* group;
    MemoryPool* codepool;
    MemoryPool* chippool;

    codepool = pool_Create(0x00200000, 0, 0, 0x3FFFFFFF);
    chippool = pool_Create(0x00000000, 0, 0, 0x3FFFFFFF);

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
    group->pools[0] = pool_Create(-1, 0, 0, 0x3FFFFFFF);

    //	Create BSS_C group
    group = group_Create("BSS_C", 1);
    group->pools[0] = pool_Create(-1, 0, 0, 0x3FFFFFFF);
}

static void
group_SetupUnbankedCommodore(uint32_t baseAddress, uint32_t size) {
    MemoryGroup* group;
    MemoryPool* codepool;

    codepool = pool_Create(0, baseAddress, 0, size);

    //	Create CODE group
    group = group_Create("CODE", 1);
    group->pools[0] = codepool;

    //	Create DATA group
    group = group_Create("DATA", 1);
    group->pools[0] = codepool;

    //	Create BSS group
    group = group_Create("BSS", 3);
    group->pools[0] = codepool;
    group->pools[1] = pool_Create(-1, 0, 0, baseAddress);
    group->pools[2] = pool_Create(-1, baseAddress + size, 0, 0x10000 - baseAddress - size);
}

void
group_SetupUnbankedCommodore128(void) {
    group_SetupUnbankedCommodore(0x1C0E, 0xF000 - 0x1C0E);
}

void
group_SetupCommodore64(void) {
    group_SetupUnbankedCommodore(0x080E, 0xA000 - 0x080E);
}

void
group_SetupCommodore264(void) {
    group_SetupUnbankedCommodore(0x100E, 0xFD00 - 0x100E);
}

static void
setupCommodore128ROM(uint32_t baseAddress, uint32_t size) {
    MemoryGroup* group;
    MemoryPool* pool;

    pool = pool_Create(0, baseAddress, 0, size);

    //	Create CODE group
    group = group_Create("CODE", 1);
    group->pools[0] = pool;

    //	Create DATA group
    group = group_Create("DATA", 1);
    group->pools[0] = pool;

    //	Create BSS group
    pool = pool_Create(-1, 0x0000, 0, 0x10000);
    group = group_Create("BSS", 1);
    group->pools[0] = pool;
}

void
group_SetupCommodore128FunctionROM() {
    setupCommodore128ROM(0x8000, 0x8000);
}

void
group_SetupCommodore128FunctionROMLow() {
    setupCommodore128ROM(0x8000, 0x4000);
}

void
group_SetupCommodore128FunctionROMHigh() {
    setupCommodore128ROM(0xC000, 0x4000);
}

void
group_SetupSegaMegaDrive() {
    MemoryGroup* group;
    MemoryPool* rompool = pool_Create(0, 0x000000, 0, 0x400000);
    MemoryPool* rampool = pool_Create(0, 0xFF0000, 0, 0x10000);

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

    MemoryPool* homepool = pool_Create(0, 0, 0, 0x400);
    MemoryPool* codepool = pool_Create(0x400, 0x400, 0, size - 16 - 0x400);

    //	Create CODE group

    group = group_Create("CODE", 2);
    group->pools[0] = homepool;
    group->pools[1] = codepool;

    //	Create HOME group

    group = group_Create("HOME", 1);
    group->pools[0] = codepool;

    //	Create DATA group

    group = group_Create("DATA", 2);
    group->pools[0] = homepool;
    group->pools[1] = codepool;

    //	Create BSS group

    group = group_Create("BSS", 1);
    group->pools[0] = pool_Create(-1, 0xC000, 0, 0x1FF8);
}

void
group_SetupSegaMasterSystemBanked(void) {
    MemoryPool* codepool;
    MemoryPool* codepools[64];
    MemoryGroup* group;

    codepool = pool_Create(0, 0, 0, 0x400);
    codepools[0] = pool_Create(0x400, 0x400, 0, 0x4000 - 0x400);
    for (int i = 1; i < 64; ++i)
        codepools[i - 1] = pool_Create(i * 0x4000, 0x8000, i, 0x4000);

    //	Create HOME group

    group = group_Create("HOME", 1);
    group->pools[0] = codepool;

    //	Create CODE group

    group = group_Create("CODE", 65);
    group->pools[0] = codepool;
    for (int i = 1; i < 65; ++i)
        group->pools[i] = codepools[i - 1];

    //	Create DATA group

    group = group_Create("DATA", 63);
    group->pools[0] = codepool;
    for (int i = 1; i < 65; ++i)
        group->pools[i] = codepools[i - 1];

    //	Create BSS group

    group = group_Create("BSS", 1);
    group->pools[0] = pool_Create(-1, 0xC000, 0, 0x1FF8);
}


void
group_SetupHC8XXROM(void) {
    MemoryGroup* group;

    MemoryPool* codepool = pool_Create(0, 0, 1, 0x4000);
	MemoryPool* bss = pool_Create(-1, 0x0000, 0x80, 0x4000);

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

    //	Create CODE_S (shared) group

    group = group_Create("CODE_S", 1);
    group->pools[0] = codepool;

    //	Create DATA_S (shared) group

    group = group_Create("DATA_S", 1);
    group->pools[0] = codepool;

    //	Create BSS_S (shared) group

    group = group_Create("BSS_S", 1);
    group->pools[0] = bss;
}


void
group_SetupHC8XXSmall(void) {
	/* HC800, CODE: 64 KiB text + data + bss */
    MemoryGroup* group;

    MemoryPool* home = pool_Create(0, 0, 0x81, 0x4000);
    MemoryPool* shared = pool_Create(0, 0x4000, 0x82, 0xC000);

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

    MemoryPool* home = pool_Create(0, 0, 0x81, 0x4000);
    MemoryPool* codeShared = pool_Create(0, 0x4000, 0x82, 0xC000);
    MemoryPool* data = pool_Create(0, 0, 0x85, 0x4000);
    MemoryPool* dataShared = pool_Create(0, 0x4000, 0x86, 0xC000);

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

    MemoryPool* home = pool_Create(0, 0, 0x81, 0x4000);
    MemoryPool* shared = pool_Create(0, 0x4000, 0x82, 0x8000);
	int firstCodeBank = 0x83;
	int codeBanks = (0x100 - firstCodeBank) / 2;
	MemoryPool* banks[HC800_MAX_BANKS];
	for (int i = 0; i < codeBanks; ++i) {
		banks[i] = pool_Create(0, 0x8000, i * 2 + firstCodeBank, 0x8000);
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

    MemoryPool* home = pool_Create(0, 0, 0x81, 0x4000);
    MemoryPool* shared = pool_Create(0, 0x4000, 0x82, 0x4000);
    MemoryPool* data = pool_Create(0, 0, 0x83, 0x4000);
    MemoryPool* dataShared = pool_Create(0, 0x4000, 0x84, 0xC000);

	int firstCodeBank = 0x87;
	int codeBanks = (0x100 - firstCodeBank) / 2;
	MemoryPool* banks[HC800_MAX_BANKS];
	for (int i = 0; i < codeBanks; ++i) {
		banks[i] = pool_Create(0, 0x8000, i * 2 + firstCodeBank, 0x8000);
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

    MemoryPool* home = pool_Create(0, 0, 0x81, 0x4000);
    MemoryPool* shared = pool_Create(0, 0x4000, 0x82, 0x4000);
	int firstCodeBank = 0x83;
	int codeBanks = (0x100 - firstCodeBank) / 2;
	MemoryPool* banks[HC800_MAX_BANKS];
	for (int i = 0; i < codeBanks; ++i) {
		banks[i] = pool_Create(0, 0x8000, i * 2 + firstCodeBank, 0x8000);
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

    MemoryPool* home = pool_Create(0, 0, 0x81, 0x4000);
    MemoryPool* sharedCode = pool_Create(0, 0x4000, 0x82, 0x4000);
	MemoryPool* bssPool = pool_Create(0, 0x0000, 0x83, 0x4000);
	MemoryPool* sharedBssPool = pool_Create(0, 0x4000, 0x84, 0xC000);
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
		group->pools[i + 2] = pool_Create(0, 0x8000, firstCodeBank + i * 2, 0x8000);
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

    MemoryPool* system_ram = pool_Create(0, 0x600, 0, 0x8000 - 0x600);

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


