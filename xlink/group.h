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

#ifndef	XLINK_GROUP_H_INCLUDED_
#define	XLINK_GROUP_H_INCLUDED_

extern void	group_SetupGameboy(void);
extern void group_SetupSmallGameboy(void);
extern void	group_SetupAmiga(void);
extern void	group_SetupCommodore64(void);
extern void	group_SetupCommodore264(void);
extern void	group_SetupUnbankedCommodore128(void);
extern void	group_SetupCommodore128FunctionROM(void);
extern void	group_SetupCommodore128FunctionROMHigh(void);
extern void	group_SetupCommodore128FunctionROMLow(void);
extern bool_t group_AllocateMemory(char* groupName, uint32_t size, int32_t bankId, int32_t* cpuByteLocation, int32_t* cpuBank, int32_t* imageLocation);
extern bool_t group_AllocateAbsolute(char* groupName, uint32_t size, int32_t bankId, int32_t cpuByteLocation, int32_t* cpuBank, int32_t* imageLocation);

#endif
