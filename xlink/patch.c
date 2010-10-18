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

#include "asmotor.h"
#include "xlink.h"
#include <math.h>

#define	fix2double(i)	((double)((i)/65536.0))
#define	double2fix(d)	((SLONG)((d)*65536.0))

#ifndef	PI
#define	PI				(acos(-1))
#endif

#define	STACKSIZE	256

static char* g_StringStack[STACKSIZE];
static SLONG g_Stack[STACKSIZE];
static SLONG g_nStackIndex;

static void PushString(char *pszValue)
{
	if(g_nStackIndex >= STACKSIZE)
		Error("patch too complex");

	g_StringStack[g_nStackIndex++] = pszValue;
}

static void push(SLONG value)
{
	if(g_nStackIndex >= STACKSIZE)
		Error("patch too complex");

	g_Stack[g_nStackIndex++] = value;
}

static SLONG pop(void)
{
	if(g_nStackIndex > 0)
		return g_Stack[--g_nStackIndex];

	Error("mangled patch");
	return 0;
}

static char* PopString(void)
{
	if(g_nStackIndex > 0)
		return g_StringStack[--g_nStackIndex];

	Error("mangled patch");
	return NULL;
}


static void	pop2(SLONG* pLeft, SLONG* pRight)
{
	*pRight = pop();
	*pLeft = pop();
}


static void	PopStrings(char** ppszLeft, char** ppszRight)
{
	*ppszRight = PopString();
	*ppszLeft = PopString();
}


static void CombinePatchStrings(char* pszOperator)
{
	char* pszLeft;
	char* pszRight;
	char* pszNewString;

	PopStrings(&pszLeft, &pszRight);

	pszNewString = (char*)malloc(strlen(pszLeft) + strlen(pszRight) + strlen(pszOperator) + 5);
	sprintf(pszNewString, "(%s)%s(%s)", pszLeft, pszOperator, pszRight);

	PushString(pszNewString);

	free(pszLeft);
	free(pszRight);
}



static void CombinePatchFunctionStrings(char* pszFunc)
{
	char* pszLeft;
	char* pszRight;
	char* pszNewString;

	PopStrings(&pszLeft, &pszRight);

	pszNewString = (char*)malloc(strlen(pszLeft) + strlen(pszRight) + strlen(pszFunc) + 4);
	sprintf(pszNewString, "%s(%s,%s)", pszFunc, pszLeft, pszRight);

	PushString(pszNewString);

	free(pszLeft);
	free(pszRight);
}


static void CombinePatchFunctionString(char* pszFunc)
{
	char* pszLeft;
	char* pszNewString;

	pszLeft = PopString();

	pszNewString = (char*)malloc(strlen(pszLeft) + strlen(pszFunc) + 3);
	sprintf(pszNewString, "%s(%s)", pszFunc, pszLeft);

	PushString(pszNewString);

	free(pszLeft);
}


char* GetPatchString(SPatch* patch, SSection* sect)
{
	SLONG size = patch->ExprSize;
	UBYTE* expr = patch->pExpr;

	g_nStackIndex = 0;

	while(size-- > 0)
	{
		char* pszLeft;
		char* pszRight;

		switch(*expr++)
		{
			case OBJ_OP_SUB:
				CombinePatchStrings("-");
				break;
			case OBJ_OP_ADD:
				CombinePatchStrings("+");
				break;
			case OBJ_OP_XOR:
				CombinePatchStrings("^");
				break;
			case OBJ_OP_OR:
				CombinePatchStrings("|");
				break;
			case OBJ_OP_AND:
				CombinePatchStrings("&");
				break;
			case OBJ_OP_SHL:
				CombinePatchStrings("<<");
				break;
			case OBJ_OP_SHR:
				CombinePatchStrings(">>");
				break;
			case OBJ_OP_MUL:
				CombinePatchStrings("*");
				break;
			case OBJ_OP_DIV:
				CombinePatchStrings("/");
				break;
			case OBJ_OP_MOD:
				CombinePatchStrings("%");
				break;
			case OBJ_OP_LOGICOR:
				CombinePatchStrings("||");
				break;
			case OBJ_OP_LOGICAND:
				CombinePatchStrings("&&");
				break;
			case OBJ_OP_LOGICNOT:
				CombinePatchFunctionString("!");
				break;
			case OBJ_OP_LOGICGE:
				CombinePatchStrings(">=");
				break;
			case OBJ_OP_LOGICGT:
				CombinePatchStrings(">");
				break;
			case OBJ_OP_LOGICLE:
				CombinePatchStrings("<=");
				break;
			case OBJ_OP_LOGICLT:
				CombinePatchStrings("<");
				break;
			case OBJ_OP_LOGICEQU:
				CombinePatchStrings("==");
				break;
			case OBJ_OP_LOGICNE:
				CombinePatchStrings("!=");
				break;
			case OBJ_FUNC_LOWLIMIT:
			{
				PopStrings(&pszLeft, &pszRight);
				PushString(pszLeft);
				free(pszRight);
				break;
			}
			case OBJ_FUNC_HIGHLIMIT:
			{
				PopStrings(&pszLeft, &pszRight);
				PushString(pszLeft);
				free(pszRight);
				break;
			}
			case OBJ_FUNC_FDIV:
				CombinePatchFunctionStrings("FDIV");
				break;
			case OBJ_FUNC_FMUL:
				CombinePatchFunctionStrings("FMUL");
				break;
			case OBJ_FUNC_ATAN2:
				CombinePatchFunctionStrings("ATAN2");
				break;
			case OBJ_FUNC_SIN:
				CombinePatchFunctionString("SIN");
				break;
			case OBJ_FUNC_COS:
				CombinePatchFunctionString("COS");
				break;
			case OBJ_FUNC_TAN:
				CombinePatchFunctionString("TAN");
				break;
			case OBJ_FUNC_ASIN:
				CombinePatchFunctionString("ASIN");
				break;
			case OBJ_FUNC_ACOS:
				CombinePatchFunctionString("ACOS");
				break;
			case OBJ_FUNC_ATAN:
				CombinePatchFunctionString("ATAN");
				break;
			case OBJ_CONSTANT:
			{
				ULONG d;
				char s[16];

				d  = (*expr++);
				d |= (*expr++) << 8;
				d |= (*expr++) << 16;
				d |= (*expr++) << 24;

				sprintf(s, "%lu", d);
				PushString(_strdup(s));

				size -= 4;
				break;
			}
			case OBJ_SYMBOL:
			{
				ULONG d;

				d  = (*expr++);
				d |= (*expr++) << 8;
				d |= (*expr++) << 16;
				d |= (*expr++) << 24;

				PushString(_strdup(sect_GetSymbolName(sect, d)));
				size -= 4;
				break;
			}
			case OBJ_FUNC_BANK:
			{
				char* pszName;
				char* pszNew;
				ULONG d;

				d  = (*expr++);
				d |= (*expr++) << 8;
				d |= (*expr++) << 16;
				d |= (*expr++) << 24;

				pszName = sect_GetSymbolName(sect, d);

				pszNew = malloc(strlen(pszName) + 7);
				sprintf(pszNew, "BANK(%s)", pszName); 

				PushString(pszNew);

				size -= 4;
				break;
			}
			case OBJ_PCREL:
			{
				CombinePatchStrings("+");
				PushString("*");
				CombinePatchStrings("-");
				break;
			}
			default:
				Error("Unknown patch operator");
				break;
		}
	}

	return PopString();
}



SLONG calc_patch(SPatch* patch, SSection* sect)
{
	SLONG size = patch->ExprSize;
	UBYTE* expr = patch->pExpr;

	g_nStackIndex = 0;

	while(size > 0)
	{
		SLONG left, right;

		--size;

		switch(*expr++)
		{
			case OBJ_OP_SUB:
			{
				pop2(&left, &right);
				push(left - right);
				break;
			}
			case OBJ_OP_ADD:
			{
				pop2(&left, &right);
				push(left + right);
				break;
			}
			case OBJ_OP_XOR:
			{
				pop2(&left, &right);
				push(left ^ right);
				break;
			}
			case OBJ_OP_OR:
			{
				pop2(&left, &right);
				push(left | right);
				break;
			}
			case OBJ_OP_AND:
			{
				pop2(&left, &right);
				push(left & right);
				break;
			}
			case OBJ_OP_SHL:
			{
				pop2(&left, &right);
				push(left << right);
				break;
			}
			case OBJ_OP_SHR:
			{
				pop2(&left, &right);
				push(left >> right);
				break;
			}
			case OBJ_OP_MUL:
			{
				pop2(&left, &right);
				push(left * right);
				break;
			}
			case OBJ_OP_DIV:
			{
				pop2(&left, &right);
				push(left / right);
				break;
			}
			case OBJ_OP_MOD:
			{
				pop2(&left, &right);
				push(left % right);
				break;
			}
			case OBJ_OP_LOGICOR:
			{
				pop2(&left, &right);
				push(left || right ? 1 : 0);
				break;
			}
			case OBJ_OP_LOGICAND:
			{
				pop2(&left, &right);
				push(left && right ? 1 : 0);
				break;
			}
			case OBJ_OP_LOGICNOT:
			{
				push(pop() ? 0 : 1);
				break;
			}
			case OBJ_OP_LOGICGE:
			{
				pop2(&left, &right);
				push(left >= right ? 1 : 0);
				break;
			}
			case OBJ_OP_LOGICGT:
			{
				pop2(&left, &right);
				push(left > right ? 1 : 0);
				break;
			}
			case OBJ_OP_LOGICLE:
			{
				pop2(&left, &right);
				push(left <= right ? 1 : 0);
				break;
			}
			case OBJ_OP_LOGICLT:
			{
				pop2(&left, &right);
				push(left < right ? 1 : 0);
				break;
			}
			case OBJ_OP_LOGICEQU:
			{
				pop2(&left, &right);
				push(left == right ? 1 : 0);
				break;
			}
			case OBJ_OP_LOGICNE:
			{
				pop2(&left, &right);
				push(left != right ? 1 : 0);
				break;
			}
			case OBJ_FUNC_LOWLIMIT:
			{
				pop2(&left, &right);
				if(left >= right)
					push(left);
				else
					Error("Expression \"%s\" at offset %d in section \"%s\" out of range", GetPatchString(patch, sect), patch->Offset, sect->Name);
				break;
			}
			case OBJ_FUNC_HIGHLIMIT:
			{
				pop2(&left, &right);
				if(left <= right)
					push(left);
				else
					Error("Expression \"%s\" at offset %d in section \"%s\" out of range", GetPatchString(patch, sect), patch->Offset, sect->Name);
				break;
			}
			case OBJ_FUNC_FDIV:
			{
				pop2(&left, &right);
				push(imuldiv(left, 65536, right));
				break;
			}
			case OBJ_FUNC_FMUL:
			{
				pop2(&left, &right);
				push(imuldiv(left, right, 65536));
				break;
			}
			case OBJ_FUNC_ATAN2:
			{
				pop2(&left, &right);
				push(double2fix(atan2(fix2double(left), fix2double(right)) / 2 / PI * 65536));
				break;
			}
			case OBJ_FUNC_SIN:
			{
				push(double2fix(sin(fix2double(pop())) * 2 * PI / 65536));
				break;
			}
			case OBJ_FUNC_COS:
			{
				push(double2fix(cos(fix2double(pop())) * 2 * PI / 65536));
				break;
			}
			case OBJ_FUNC_TAN:
			{
				push(double2fix(tan(fix2double(pop())) * 2 * PI / 65536));
				break;
			}
			case OBJ_FUNC_ASIN:
			{
				push(double2fix(asin(fix2double(pop())) / 2 / PI * 65536));
				break;
			}
			case OBJ_FUNC_ACOS:
			{
				push(double2fix(acos(fix2double(pop())) / 2 / PI * 65536));
				break;
			}
			case OBJ_FUNC_ATAN:
			{
				push(double2fix(atan(fix2double(pop())) / 2 / PI * 65536));
				break;
			}
			case OBJ_CONSTANT:
			{
				ULONG d;

				d  = (*expr++);
				d |= (*expr++) << 8;
				d |= (*expr++) << 16;
				d |= (*expr++) << 24;

				push(d);
				size -= 4;
				break;
			}
			case OBJ_SYMBOL:
			{
				ULONG d;

				d  = (*expr++);
				d |= (*expr++) << 8;
				d |= (*expr++) << 16;
				d |= (*expr++) << 24;

				push(sect_GetSymbolValue(sect, d));
				size -= 4;
				break;
			}
			case OBJ_FUNC_BANK:
			{
				ULONG d;

				d  = (*expr++);
				d |= (*expr++) << 8;
				d |= (*expr++) << 16;
				d |= (*expr++) << 24;

				push(sect_GetSymbolBank(sect, d));
				size -= 4;
				break;
			}
			case OBJ_PCREL:
			{
				pop2(&left, &right);
				push(left + right - (patch->Offset + sect->ImageOffset));
				break;
			}
			default:
				Error("Unknown patch operator");
				break;
		}
	}

	return pop();
}

static void do_section(SSection* sect)
{
	SPatches* patches = sect->pPatches;

	if(patches != NULL)
	{
		ULONG i;

		for(i = 0; i < patches->TotalPatches; ++i)
		{
			SPatch* patch = &patches->Patches[i];
			SLONG value = calc_patch(patch, sect);

			switch(patch->Type)
			{
				case PATCH_BYTE:
				{
					if(value >= -128 && value <= 255)
						sect->pData[patch->Offset] = (UBYTE)value;
					else
						Error("Expression \"%s\" at offset %d in section \"%s\" out of range", GetPatchString(patch, sect), patch->Offset, sect->Name);
					break;
				}
				case PATCH_LWORD:
				{
					if(value >= -32768 && value <= 65535)
					{
						sect->pData[patch->Offset + 0] = (UBYTE)value;
						sect->pData[patch->Offset + 1] = (UBYTE)(value >> 8);
					}
					else
					{
						Error("patch out of range");
					}
					break;
				}
				case	PATCH_BWORD:
				{
					if(value>=-32768 && value<=65535)
					{
						sect->pData[patch->Offset+0] = (UBYTE)(value>>8);
						sect->pData[patch->Offset+1] = (UBYTE)value;
					}
					else
					{
						Error("patch out of range");
					}
					break;
				}
				case	PATCH_LLONG:
				{
					sect->pData[patch->Offset+0] = (UBYTE)value;
					sect->pData[patch->Offset+1] = (UBYTE)(value>>8);
					sect->pData[patch->Offset+2] = (UBYTE)(value>>16);
					sect->pData[patch->Offset+3] = (UBYTE)(value>>24);
					break;
				}
				case	PATCH_BLONG:
				{
					sect->pData[patch->Offset+0] = (UBYTE)(value>>24);
					sect->pData[patch->Offset+1] = (UBYTE)(value>>16);
					sect->pData[patch->Offset+2] = (UBYTE)(value>>8);
					sect->pData[patch->Offset+3] = (UBYTE)value;
					break;
				}
				default:
				{
					Error("unhandled patch type");
				}
			}
		}
	}
}


void	patch_Process(void)
{
	SSection* sect;

	sect=pSections;

	while(sect)
	{
		if(sect->Used)
		{
			do_section(sect);
		}
		sect=sect->pNext;
	}
}
