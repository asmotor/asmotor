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

#ifndef	OBJECT_H
#define	OBJECT_H

typedef	enum
{
	GROUP_TEXT=0,
	GROUP_BSS=1
}	EGroupType;

typedef	struct
{
	char		Name[MAXSYMNAMELENGTH];
	EGroupType	Type;
}	SGroup;

typedef	struct
{
	uint32_t	TotalGroups;
	SGroup	Groups[];
}	SGroups;

extern	void	obj_Read(char* name);

#endif
