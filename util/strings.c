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

	return l1 - l2;
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

	return l1 - l2;
}
#endif

