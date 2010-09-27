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
 * xLib - MAIN.C
 * Copyright 1996-1998 Carsten Sorensen (csorensen@ea.com)
 *
 *	ULONG	"XLB\0"
 *	ULONG	TotalFiles
 *	REPT	TotalFiles
 *		ASCIIZ	Name
 *		ULONG	Time
 *		ULONG	Date
 *		ULONG	Size
 *		UBYTE	Data[Size]
 *	ENDR
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../types.h"
#include "libwrap.h"

extern	void	fatalerror(char* s);

SLONG	file_Length(FILE* f)
{
	ULONG	r,
			p;

	p=ftell(f);
	fseek(f, 0, SEEK_END);
	r=ftell(f);
	fseek(f, p, SEEK_SET);

	return r;
}

SLONG file_ReadASCIIz(char* b, FILE* f)
{
	SLONG r = 0;

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

UWORD	file_ReadWord(FILE* f)
{
	UWORD	r;

	r  = (UWORD)fgetc(f);
	r |= (UWORD)(fgetc(f)<<8);

	return r;
}

void	file_WriteWord(UWORD w, FILE* f)
{
	fputc(w, f);
	fputc(w>>8, f);
}

ULONG	file_ReadLong(FILE* f)
{
	ULONG	r;

	r =fgetc(f);
	r|=fgetc(f)<<8;
	r|=fgetc(f)<<16;
	r|=fgetc(f)<<24;

	return r;
}

void	file_WriteLong(ULONG w, FILE* f)
{
	fputc(w, f);
	fputc(w>>8, f);
	fputc(w>>16, f);
	fputc(w>>24, f);
}

SLibrary* lib_ReadLib0(FILE* f, SLONG size)
{
	if(size)
	{
		SLibrary* l=NULL,
					*first=NULL;

		file_ReadLong(f);	size-=4;	//	Skip count

		while(size>0)
		{
			if(l==NULL)
			{
				if((l=(SLibrary* )malloc(sizeof(SLibrary)))==NULL)
					fatalerror("Out of memory");

				first=l;
			}
			else
			{
				if((l->pNext=(SLibrary* )malloc(sizeof(SLibrary)))==NULL)
					fatalerror("Out of memory");
				l=l->pNext;
			}

			size-=file_ReadASCIIz(l->tName, f);
			l->ulTime=file_ReadLong(f); size-=4;
			l->ulDate=file_ReadLong(f); size-=4;
			l->nByteLength=file_ReadLong(f); size-=4;
			if((l->pData = (UBYTE* )malloc(l->nByteLength)) != NULL)
			{
				fread(l->pData, sizeof(UBYTE), l->nByteLength, f);
				size-=l->nByteLength;
			}
			else
				fatalerror("Out of memory");

			l->pNext=NULL;
		}
		return first;
	}

	return NULL;
}

SLibrary* lib_Read(char* filename)
{
	FILE		*f;

	if((f = fopen(filename,"rb")) != NULL)
	{
		SLONG		size;
		char		ID[5];

		size=file_Length(f);
		if(size==0)
		{
			fclose(f);
			return NULL;
		}

		fread(ID, sizeof(char), 4, f);
		ID[4]=0;
		size-=4;

		if(strcmp(ID,"XLB\0")==0)
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

BOOL lib_Write(SLibrary* lib, char* filename)
{
	FILE* f;

	if((f = fopen(filename,"wb")) != NULL)
	{
		ULONG count = 0;

		fwrite("XLB\0", sizeof(char), 4, f);
		file_WriteLong(0, f);

		while(lib)
		{
			file_WriteASCIIz(lib->tName, f);
			file_WriteLong(lib->ulTime, f);
			file_WriteLong(lib->ulDate, f);
			file_WriteLong(lib->nByteLength, f);
			fwrite(lib->pData, sizeof(UBYTE), lib->nByteLength,f);
			lib = lib->pNext;
			++count;
		}

		fseek(f, 4, SEEK_SET);
		file_WriteLong(count, f);

		fclose(f);
		/*printf("Library '%s' closed\n", filename);*/
		return TRUE;
	}

	return FALSE;
}

void	TruncateFileName(char* dest, char* src)
{
	SLONG l;

	l = (SLONG)strlen(src) - 1;
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

		if((module=lib_Find(lib,filename))==NULL)
		{
			if((module = (SLibrary* )malloc(sizeof(SLibrary))) != NULL)
			{
				module->pNext=lib;
				lib=module;
			}
			else
				fatalerror("Out of memory");
		}
		else
		{
			/* Module already exists */
			free(module->pData);
		}

		module->nByteLength=file_Length(f);
		strcpy(module->tName, truncname);
		if((module->pData = (UBYTE* )malloc(module->nByteLength)) != NULL)
		{
			fread(module->pData, sizeof(UBYTE), module->nByteLength, f);
		}

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
	BOOL found = 0;

	first = pp = &lib;

	TruncateFileName(truncname, filename);
	while(*pp != NULL && !found)
	{
		if(strcmp((*pp)->tName, truncname) == 0)
		{
			SLibrary* t = *pp;

			if(t->pData)
				free(t->pData);

			*pp = t->pNext;

			free(t);
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
			free(lib->pData);

		l=lib;
		lib=lib->pNext;
		free(l);
	}
}