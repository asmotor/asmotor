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

#ifndef XLINK_GROUP_H_INCLUDED_
#define XLINK_GROUP_H_INCLUDED_

#include "types.h"

#include "symbol.h"
#include <stdbool.h>

struct MemoryChunk_;

typedef struct {
    int32_t imageLocation;      // This pool's position in the ROM image, -1 if not written
    uint32_t overlay;           // Which overlay file in which the pool should reside
    uint32_t cpuByteLocation;   // Where the CPU sees this pool in its address space
    int32_t cpuBank;            // What the CPU calls this bank
    uint32_t size;              // Size of pool seen from the CPU
	bool onlyAbs;				// Only allow absolute placement, never allocate dynamically

    struct MemoryChunk_* freeChunks;
} MemoryPool;

typedef struct MemoryGroup_ {
    struct MemoryGroup_* nextGroup;

    char name[MAX_SYMBOL_NAME_LENGTH];
    int32_t totalPools;

    MemoryPool* pools[];
} MemoryGroup;


extern MemoryPool*
pool_Create(int32_t imageLocation, uint32_t overlay, uint32_t cpuByteLocation, int32_t cpuBank, uint32_t size, bool onlyAbs);

extern void
pool_Free(MemoryPool* pool);

extern MemoryGroup*
group_Create(const char* groupName, uint32_t totalBanks);

extern void
group_InitMemoryChunks(void);

extern void
group_SetupGameboy(void);

extern void
group_SetupSmallGameboy(void);

extern void
group_SetupAmiga(void);

extern void
group_SetupSegaMegaDrive(void);

extern void
group_SetupSegaMasterSystem(int size);

extern void
group_SetupSegaMasterSystemBanked(void);

void
group_SetupHC8XXROM(void);

void
group_SetupHC8XXSmall(void);

void
group_SetupHC8XXSmallHarvard(void);

void
group_SetupHC8XXMedium(void);

void
group_SetupHC8XXMediumHarvard(void);

void
group_SetupHC8XXLarge(void);

extern void
group_SetupCoCo(void);

extern bool
group_AllocateMemory(const char* groupName, uint32_t size, int32_t bankId, int32_t* cpuByteLocation, int32_t* cpuBank, int32_t* imageLocation);

extern bool
group_AllocateAbsolute(const char* groupName, uint32_t size, int32_t bankId, int32_t cpuByteLocation, int32_t* cpuBank, int32_t* imageLocation);

extern bool
group_AllocateAligned(const char* groupName, uint32_t size, int32_t bankId, int32_t byteAlign, int32_t* cpuByteLocation, int32_t* cpuBank, int32_t* imageLocation);

extern bool
group_NeedsOverlay(void);

#endif
