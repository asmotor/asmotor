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

#if defined(__VBCC__) || defined(__GNUC__)
#include "asmotor.h"

#include <ctype.h>
#include <string.h>

#include "mem.h"

char* _strdup(const char* pszString)
{
	int l = strlen(pszString);
	char* r = mem_Alloc(l + 1);
	memcpy(r, pszString, l + 1);
	return r;
}

char* _strupr(char* pszString)
{
	char* r = pszString;
	while(*r)
	{
		*r = toupper(*r);
		++r;
	}
	return pszString;
}

char* _strlwr(char* pszString)
{
	char* r = pszString;
	while(*r)
	{
		*r = tolower(*r);
		++r;
	}
	return pszString;
}

int _strnicmp(const char* pszString1, const char* pszString2, int nCount)
{
	char l1 = 0;
	char l2 = 0;

	while(*pszString1 && *pszString2 && nCount-- > 0)
	{
		l1 = tolower(*pszString1++);
		l2 = tolower(*pszString2++);

		if(l1 != l2)
			return l1 - l2;
	}

	if(nCount == 0)
		return 0;

	return *pszString1 - *pszString2;
}

int _stricmp(const char* pszString1, const char* pszString2)
{
	char l1 = 0;
	char l2 = 0;

	while(*pszString1 && *pszString2)
	{
		l1 = tolower(*pszString1++);
		l2 = tolower(*pszString2++);

		if(l1 != l2)
			return l1 - l2;
	}

	return *pszString1 - *pszString2;
}
#endif

