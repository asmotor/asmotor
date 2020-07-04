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

#include "mem.h"
#include "str.h"
#include "types.h"

#include "symbol.h"
#include "xlink.h"

#include <string.h>

typedef struct MemoryChunk_ {
    struct MemoryChunk_* nextChunk;

    uint32_t cpuByteLocation;
    uint32_t size;
} MemoryChunk;

typedef struct {
    int32_t imageLocation;      // This pool's position in the ROM image, -1 if not written
    uint32_t cpuByteLocation;   // Where the CPU sees this pool in its address space
    int32_t cpuBank;            // What the CPU calls this bank
    uint32_t size;              // Size of pool seen from the CPU

    MemoryChunk* freeChunks;
} MemoryPool;

typedef struct MemoryGroup_ {
    struct MemoryGroup_* nextGroup;

    char name[MAX_SYMBOL_NAME_LENGTH];
    int32_t totalPools;

    MemoryPool* pools[];
} MemoryGroup;

static MemoryGroup* s_machineGroups = NULL;

static MemoryGroup*
group_Create(const char* groupName, uint32_t totalBanks) {
    MemoryGroup** ppgroup = &s_machineGroups;
    while (*ppgroup)
        ppgroup = &(*ppgroup)->nextGroup;

    *ppgroup = (MemoryGroup*) mem_Alloc(sizeof(MemoryGroup) + sizeof(MemoryPool*) * totalBanks);

    strncpy((*ppgroup)->name, groupName, sizeof((*ppgroup)->name));
    (*ppgroup)->nextGroup = NULL;
    (*ppgroup)->totalPools = totalBanks;

    return *ppgroup;
}

static MemoryPool*
pool_Create(int32_t imageLocation, uint32_t cpuByteLocation, int32_t cpuBank, uint32_t size) {
    MemoryPool* pool = (MemoryPool*) mem_Alloc(sizeof(MemoryPool));

    pool->imageLocation = imageLocation;
    pool->cpuByteLocation = cpuByteLocation;
    pool->cpuBank = cpuBank;
    pool->size = size;

    pool->freeChunks = NULL;

    return pool;
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

static bool
pool_AllocateAbsolute(MemoryPool* pool, uint32_t size, uint32_t cpuByteLocation) {
    for (MemoryChunk* chunk = pool->freeChunks; chunk != NULL; chunk = chunk->nextChunk) {
        if (cpuByteLocation >= chunk->cpuByteLocation
            && cpuByteLocation + size <= chunk->cpuByteLocation + chunk->size) {

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

static void
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
                *imageLocation =
                        pool->imageLocation == -1 ? -1 : pool->imageLocation + *cpuByteLocation - (int32_t) pool->cpuByteLocation;
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

    //	initialise memory chunks

    group_InitMemoryChunks();
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

    //	initialise memory chunks

    group_InitMemoryChunks();
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

    //	initialise memory chunks
    group_InitMemoryChunks();
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

    //	initialise memory chunks
    group_InitMemoryChunks();
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

    //	initialise memory chunks
    group_InitMemoryChunks();
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

    //	initialise memory chunks
    group_InitMemoryChunks();
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

    //	initialise memory chunks

    group_InitMemoryChunks();
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

    //	initialise memory chunks

    group_InitMemoryChunks();
}


void
group_SetupHC8XXROM(void) {
    MemoryPool* codepool;
    MemoryGroup* group;

    codepool = pool_Create(0, 0, 0, 0x4000);

    //	Create HOME group

    group = group_Create("HOME", 1);
    group->pools[0] = codepool;

    //	Create CODE group

    group = group_Create("CODE", 1);
    group->pools[0] = codepool;

    //	Create DATA group

    //	Create BSS group

    group = group_Create("BSS", 1);
    group->pools[0] = pool_Create(-1, 0x0000, 0, 0x4000);

    //	initialise memory chunks

    group_InitMemoryChunks();
}


void
group_SetupHC8XXCom(void) {
    MemoryPool* codepool;
    MemoryGroup* group;

    codepool = pool_Create(0, 0, 0, 0x10000);

    //	Create HOME group

    group = group_Create("HOME", 1);
    group->pools[0] = codepool;

    //	Create CODE group

    group = group_Create("CODE", 1);
    group->pools[0] = codepool;

    //	Create DATA group

    //	Create BSS group

    group = group_Create("BSS", 1);
    group->pools[0] = pool_Create(-1, 0x4000, 0, 0xC000);

    //	initialise memory chunks

    group_InitMemoryChunks();
}

