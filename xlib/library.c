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

/*
 *	uint32_t	"XLB0"
 *	uint32_t	TotalFiles
 *	REPT	TotalFiles
 *		ASCIIZ	Name
 *		uint32_t	Time
 *		uint32_t	Date
 *		uint32_t	Size
 *		uint8_t	Data[Size]
 *	ENDR
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asmotor.h"
#include "mem.h"
#include "types.h"
#include "libwrap.h"

extern	void	fatalerror(char* s);

int32_t	file_Length(FILE* f)
{
	uint32_t	r,
			p;

	p=ftell(f);
	fseek(f, 0, SEEK_END);
	r=ftell(f);
	fseek(f, p, SEEK_SET);

	return r;
}

int32_t file_ReadASCIIz(char* b, FILE* f)
{
	int32_t r = 0;

	while((*b++ = (char)fgetc(f)) != 0)
	{
		++r;
	}

	return r + 1;
}

void	file_WriteASCIIz(char* b, FILE* f)
{
	while(*b)
		fputc(*b++,f);

	fputc(0, f);
}

uint16_t	file_ReadWord(FILE* f)
{
	uint16_t	r;

	r  = (uint16_t)fgetc(f);
	r |= (uint16_t)(fgetc(f)<<8);

	return r;
}

void	file_WriteWord(uint16_t w, FILE* f)
{
	fputc(w, f);
	fputc(w>>8, f);
}

uint32_t	file_ReadLong(FILE* f)
{
	uint32_t	r;

	r =fgetc(f);
	r|=fgetc(f)<<8;
	r|=fgetc(f)<<16;
	r|=fgetc(f)<<24;

	return r;
}

void	file_WriteLong(uint32_t w, FILE* f)
{
	fputc(w, f);
	fputc(w>>8, f);
	fputc(w>>16, f);
	fputc(w>>24, f);
}

SLibrary* lib_ReadLib0(FILE* f, int32_t size)
{
	if(size)
	{
		SLibrary* l = NULL;
		SLibrary* first = NULL;

		file_ReadLong(f);
		size -= 4;	//	Skip count

		while(size>0)
		{
			if(l == NULL)
			{
				first = l = (SLibrary*)mem_Alloc(sizeof(SLibrary));
			}
			else
			{
				l->pNext = (SLibrary*)mem_Alloc(sizeof(SLibrary));
				l = l->pNext;
			}

			size -= file_ReadASCIIz(l->tName, f);
			l->ulTime = file_ReadLong(f); size-=4;
			l->ulDate = file_ReadLong(f); size-=4;
			l->nByteLength = file_ReadLong(f); size-=4;
			
			l->pData = (uint8_t*)mem_Alloc(l->nByteLength);
			if(l->nByteLength != fread(l->pData, sizeof(uint8_t), l->nByteLength, f))
				fatalerror("File read failed");
			size -= l->nByteLength;

			l->pNext = NULL;
		}
		return first;
	}

	return NULL;
}

SLibrary* lib_Read(char* filename)
{
	FILE* f;

	if((f = fopen(filename,"rb")) != NULL)
	{
		int32_t size;
		char ID[5];

		size = file_Length(f);
		if(size == 0)
		{
			fclose(f);
			return NULL;
		}

		if(4 != fread(ID, sizeof(char), 4, f))
			internalerror("File read failed");
		ID[4] = 0;
		size -= 4;

		if(strcmp(ID, "XLB0") == 0)
		{
			SLibrary* r;

			r = lib_ReadLib0(f, size);
			fclose(f);
			/*printf("Library '%s' opened\n", filename);*/
			return r;
		}
		else
		{
			fclose(f);
			fatalerror("Not a valid xLib library");
			return NULL;
		}
	}
	else
	{
		/*printf("Library '%s' not found, it will be created if necessary\n", filename);*/
		return NULL;
	}
}

bool_t lib_Write(SLibrary* lib, char* filename)
{
	FILE* f;

	if((f = fopen(filename,"wb")) != NULL)
	{
		uint32_t count = 0;

		fwrite("XLB0", sizeof(char), 4, f);
		file_WriteLong(0, f);

		while(lib)
		{
			file_WriteASCIIz(lib->tName, f);
			file_WriteLong(lib->ulTime, f);
			file_WriteLong(lib->ulDate, f);
			file_WriteLong(lib->nByteLength, f);
			fwrite(lib->pData, sizeof(uint8_t), lib->nByteLength,f);
			lib = lib->pNext;
			++count;
		}

		fseek(f, 4, SEEK_SET);
		file_WriteLong(count, f);

		fclose(f);
		/*printf("Library '%s' closed\n", filename);*/
		return true;
	}

	return false;
}

void TruncateFileName(char* dest, char* src)
{
	int32_t l;

	l = (int32_t)strlen(src) - 1;
	while((l >= 0) && (src[l] != '\\') && (src[l] != '/'))
		--l;

	strcpy(dest, &src[l + 1]);
}

SLibrary* lib_Find(SLibrary* lib, char* filename)
{
	char truncname[MAXNAMELENGTH];

	TruncateFileName(truncname, filename);

	while(lib)
	{
		if(strcmp(lib->tName,truncname) == 0)
			break;

		lib = lib->pNext;
	}

	return lib;
}

SLibrary* lib_AddReplace(SLibrary* lib, char* filename)
{
	FILE* f;

	if((f = fopen(filename,"rb")) != NULL)
	{
		SLibrary* module;
		char truncname[MAXNAMELENGTH];

		TruncateFileName(truncname, filename);

		if((module = lib_Find(lib,filename)) == NULL)
		{
			module = (SLibrary*)mem_Alloc(sizeof(SLibrary));
			module->pNext = lib;
			lib = module;
		}
		else
		{
			/* Module already exists */
			mem_Free(module->pData);
		}

		module->nByteLength = file_Length(f);
		strcpy(module->tName, truncname);
		module->pData = (uint8_t*)mem_Alloc(module->nByteLength);
	
		if(module->nByteLength != fread(module->pData, sizeof(uint8_t), module->nByteLength, f))
			internalerror("File read failed");

		/*printf("Added module '%s'\n", truncname);*/

		fclose(f);
	}

	return lib;
}

SLibrary* lib_DeleteModule(SLibrary* lib, char* filename)
{
	char truncname[MAXNAMELENGTH];
	SLibrary** pp;
	SLibrary** first;
	bool_t found = 0;

	first = pp = &lib;

	TruncateFileName(truncname, filename);
	while(*pp != NULL && !found)
	{
		if(strcmp((*pp)->tName, truncname) == 0)
		{
			SLibrary* t = *pp;

			if(t->pData)
				mem_Free(t->pData);

			*pp = t->pNext;

			mem_Free(t);
			found = 1;
		}
		pp = &(*pp)->pNext;
	}

	if(!found)
		fatalerror("Module not found");
	/*
	else
		printf("Module '%s' deleted from library\n", truncname);
		*/

	return *first;
}

void	lib_Free(SLibrary* lib)
{
	while(lib)
	{
		SLibrary* l;

		if(lib->pData)
			mem_Free(lib->pData);

		l=lib;
		lib=lib->pNext;
		mem_Free(l);
	}
}
