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

#include "xlink.h"

void assign_Process(void)
{
	SSection* sect;

	/*
		Assign all bank AND org fixed sections
	 */
	for(sect = pSections; sect != NULL; sect = sect->pNext)
	{
		if(sect->Used && !sect->Assigned && sect->Bank != -1 && sect->Position != -1)
			group_Alloc(sect);
	}

	/*
		Assign all bank OR org fixed sections
	 */
	for(sect = pSections; sect != NULL; sect = sect->pNext)
	{
		if(sect->Used && !sect->Assigned &&	(sect->Bank != -1 || sect->Position != -1))
			group_Alloc(sect);
	}

	/*
		Byte aligned sections should go here
	 */

	 /*
		Assign the rest
	 */
	for(sect = pSections; sect != NULL; sect = sect->pNext)
	{
		if(sect->Used && !sect->Assigned)
			group_Alloc(sect);
	}
}
