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

#ifndef	GROUP_H
#define	GROUP_H

typedef	struct	_SMemChunk
{
	ULONG	Org;
	ULONG	Size;
	struct	_SMemChunk* pNext;
}	SMemChunk;

typedef	struct	_SMemoryPool
{
	SLONG	ImageOffset;		//	This pool's position in the ROM image, -1 if not written
	ULONG	AddressingOffset;	//	Where the CPU sees this pool in its address space
	SLONG	BankId;				//	What the CPU calls this bank
	ULONG	Size;
	ULONG	Available;
	SMemChunk				*pFreeChunks;
	//SMachineGroup			*pGroups;
	//struct	_SMemoryPool* pNext;
}	SMemoryPool;

typedef	struct	_SMachineGroup
{
	char					Name[MAXSYMNAMELENGTH];
	SLONG					TotalPools;
	struct	_SMachineGroup* pNext;
	SMemoryPool				*Pool[];
}	SMachineGroup;

extern	void	group_SetupGameboy(void);
extern	void	group_SetupSmallGameboy(void);
extern	void	group_Alloc(SSection* sect);


#endif
