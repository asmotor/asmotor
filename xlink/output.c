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

#define WRITE_BLOCK_SIZE 65536
static char* g_pszOutputFilename = NULL;

void output_SetFilename(char* name)
{
	g_pszOutputFilename = name;
}

void output_WriteRomImage(void)
{
	char* data;
	FILE* f;
	SSection* pSect;
	ULONG fsize = 0;

	data = (char*)malloc(WRITE_BLOCK_SIZE);
	memset(data, 0, WRITE_BLOCK_SIZE);
	if(data == NULL)
		Error("Out of memory");

	f = fopen(g_pszOutputFilename, "wb");
	if(f == NULL)
		Error("Unable to open \"%s\" for writing", g_pszOutputFilename);

	for(pSect = pSections; pSect != NULL; pSect = pSect->pNext)
	{
		//	This is a special exported EQU symbol section
		if(pSect->GroupID == -1)
			continue;

		if(pSect->Used && pSect->Assigned && pSect->ImageOffset != -1)
		{
			ULONG offset = pSect->ImageOffset;
			if(offset > fsize)
			{
				ULONG totaltowrite = offset - fsize;
				fseek(f, fsize, SEEK_SET);
				while(totaltowrite > 0)
				{
					ULONG towrite = totaltowrite > WRITE_BLOCK_SIZE ? WRITE_BLOCK_SIZE : totaltowrite;
					if(towrite != fwrite(data, 1, towrite, f))
						Error("Disk possibly full");
					totaltowrite -= towrite;
				}
			}
			fsize = offset;
			fwrite(pSect->pData, 1, pSect->Size, f);
			fsize += pSect->Size;
		}
	}

	fclose(f);
	free(data);
}
