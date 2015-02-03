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
#include <string.h>

#define	STACKSIZE	256

static char* s_stringStack[STACKSIZE];
static int32_t s_stack[STACKSIZE];
static int32_t s_stackIndex;

static void pushString(char* stringValue)
{
	if (s_stackIndex >= STACKSIZE)
		Error("patch too complex");

	s_stringStack[s_stackIndex++] = stringValue;
}


static void pushStringCopy(char* stringValue)
{
	char* copy = mem_Alloc(strlen(stringValue) + 1);
	strcpy(copy, stringValue);
	pushString(copy);
}


static void pushIntAsString(uint32_t intValue)
{
	char* value = mem_Alloc(16);

	sprintf(value, "%u", intValue);
	pushString(value);
}


static char* popString(void)
{
	if(s_stackIndex > 0)
		return s_stringStack[--s_stackIndex];

	Error("mangled patch");
	return NULL;
}


static void pushInt(int32_t value)
{
	if (s_stackIndex >= STACKSIZE)
		Error("patch too complex");

	s_stack[s_stackIndex++] = value;
}

static int32_t popInt(void)
{
	if (s_stackIndex > 0)
		return s_stack[--s_stackIndex];

	Error("mangled patch");
	return 0;
}


static void	popInt2(int32_t* outLeft, int32_t* outRight)
{
	*outRight = popInt();
	*outLeft = popInt();
}


static void	popString2(char** outLeft, char** outRight)
{
	*outRight = popString();
	*outLeft = popString();
}


static void combinePatchStrings(char* operator)
{
	char* left;
	char* right;
	char* result;

	popString2(&left, &right);

	result = (char*)mem_Alloc(strlen(left) + strlen(right) + strlen(operator) + 5);
	sprintf(result, "(%s)%s(%s)", left, operator, right);

	pushString(result);

	mem_Free(left);
	mem_Free(right);
}



static void combinePatchFunctionStrings(char* function)
{
	char* left;
	char* right;
	char* result;

	popString2(&left, &right);

	result = (char*)mem_Alloc(strlen(left) + strlen(right) + strlen(function) + 4);
	sprintf(result, "%s(%s,%s)", function, left, right);

	pushString(result);

	mem_Free(left);
	mem_Free(right);
}


static void combinePatchFunctionString(char* function)
{
	char* left;
	char* result;

	left = popString();

	result = (char*)mem_Alloc(strlen(left) + strlen(function) + 3);
	sprintf(result, "%s(%s)", function, left);

	pushString(result);

	mem_Free(left);
}


static char* makePatchString(Patch* patch, Section* section)
{
	int32_t size = patch->expressionSize;
	uint8_t* expression = patch->expression;

	s_stackIndex = 0;

	while (size-- > 0)
	{
		char* left;
		char* right;

		switch (*expression++)
		{
			case OBJ_OP_SUB:
				combinePatchStrings("-");
				break;
			case OBJ_OP_ADD:
				combinePatchStrings("+");
				break;
			case OBJ_OP_XOR:
				combinePatchStrings("^");
				break;
			case OBJ_OP_OR:
				combinePatchStrings("|");
				break;
			case OBJ_OP_AND:
				combinePatchStrings("&");
				break;
			case OBJ_OP_SHL:
				combinePatchStrings("<<");
				break;
			case OBJ_OP_SHR:
				combinePatchStrings(">>");
				break;
			case OBJ_OP_MUL:
				combinePatchStrings("*");
				break;
			case OBJ_OP_DIV:
				combinePatchStrings("/");
				break;
			case OBJ_OP_MOD:
				combinePatchStrings("%");
				break;
			case OBJ_OP_LOGICOR:
				combinePatchStrings("||");
				break;
			case OBJ_OP_LOGICAND:
				combinePatchStrings("&&");
				break;
			case OBJ_OP_LOGICNOT:
				combinePatchFunctionString("!");
				break;
			case OBJ_OP_LOGICGE:
				combinePatchStrings(">=");
				break;
			case OBJ_OP_LOGICGT:
				combinePatchStrings(">");
				break;
			case OBJ_OP_LOGICLE:
				combinePatchStrings("<=");
				break;
			case OBJ_OP_LOGICLT:
				combinePatchStrings("<");
				break;
			case OBJ_OP_LOGICEQU:
				combinePatchStrings("==");
				break;
			case OBJ_OP_LOGICNE:
				combinePatchStrings("!=");
				break;
			case OBJ_FUNC_LOWLIMIT:
			{
				popString2(&left, &right);
				pushString(left);
				mem_Free(right);
				break;
			}
			case OBJ_FUNC_HIGHLIMIT:
			{
				popString2(&left, &right);
				pushString(left);
				mem_Free(right);
				break;
			}
			case OBJ_FUNC_FDIV:
				combinePatchFunctionStrings("FDIV");
				break;
			case OBJ_FUNC_FMUL:
				combinePatchFunctionStrings("FMUL");
				break;
			case OBJ_FUNC_ATAN2:
				combinePatchFunctionStrings("ATAN2");
				break;
			case OBJ_FUNC_SIN:
				combinePatchFunctionString("SIN");
				break;
			case OBJ_FUNC_COS:
				combinePatchFunctionString("COS");
				break;
			case OBJ_FUNC_TAN:
				combinePatchFunctionString("TAN");
				break;
			case OBJ_FUNC_ASIN:
				combinePatchFunctionString("ASIN");
				break;
			case OBJ_FUNC_ACOS:
				combinePatchFunctionString("ACOS");
				break;
			case OBJ_FUNC_ATAN:
				combinePatchFunctionString("ATAN");
				break;
			case OBJ_CONSTANT:
			{
				uint32_t value;

				value  = (*expression++);
				value |= (*expression++) << 8;
				value |= (*expression++) << 16;
				value |= (*expression++) << 24;

				pushIntAsString(value);

				size -= 4;
				break;
			}
			case OBJ_SYMBOL:
			{
				uint32_t symbolId;

				symbolId  = (*expression++);
				symbolId |= (*expression++) << 8;
				symbolId |= (*expression++) << 16;
				symbolId |= (*expression++) << 24;

				pushStringCopy(sect_GetSymbolName(section, symbolId));

				size -= 4;
				break;
			}
			case OBJ_FUNC_BANK:
			{
				char* symbolName;
				char* copy;
				uint32_t symbolId;

				symbolId  = (*expression++);
				symbolId |= (*expression++) << 8;
				symbolId |= (*expression++) << 16;
				symbolId |= (*expression++) << 24;

				symbolName = sect_GetSymbolName(section, symbolId);

				copy = mem_Alloc(strlen(symbolName) + 7);
				sprintf(copy, "BANK(%s)", symbolName); 

				pushString(copy);

				size -= 4;
				break;
			}
			case OBJ_PCREL:
			{
				combinePatchStrings("+");
				pushString("*");
				combinePatchStrings("-");
				break;
			}
			default:
				Error("Unknown patch operator");
				break;
		}
	}

	return popString();
}


int32_t calculatePatchValue(Patch* patch, Section* section)
{
	int32_t size = patch->expressionSize;
	uint8_t* expression = patch->expression;

	s_stackIndex = 0;

	while (size > 0)
	{
		int32_t left, right;

		--size;

		switch(*expression++)
		{
			case OBJ_OP_SUB:
			{
				popInt2(&left, &right);
				pushInt(left - right);
				break;
			}
			case OBJ_OP_ADD:
			{
				popInt2(&left, &right);
				pushInt(left + right);
				break;
			}
			case OBJ_OP_XOR:
			{
				popInt2(&left, &right);
				pushInt(left ^ right);
				break;
			}
			case OBJ_OP_OR:
			{
				popInt2(&left, &right);
				pushInt(left | right);
				break;
			}
			case OBJ_OP_AND:
			{
				popInt2(&left, &right);
				pushInt(left & right);
				break;
			}
			case OBJ_OP_SHL:
			{
				popInt2(&left, &right);
				pushInt(left << right);
				break;
			}
			case OBJ_OP_SHR:
			{
				popInt2(&left, &right);
				pushInt(left >> right);
				break;
			}
			case OBJ_OP_MUL:
			{
				popInt2(&left, &right);
				pushInt(left * right);
				break;
			}
			case OBJ_OP_DIV:
			{
				popInt2(&left, &right);
				pushInt(left / right);
				break;
			}
			case OBJ_OP_MOD:
			{
				popInt2(&left, &right);
				pushInt(left % right);
				break;
			}
			case OBJ_OP_LOGICOR:
			{
				popInt2(&left, &right);
				pushInt(left || right ? 1 : 0);
				break;
			}
			case OBJ_OP_LOGICAND:
			{
				popInt2(&left, &right);
				pushInt(left && right ? 1 : 0);
				break;
			}
			case OBJ_OP_LOGICNOT:
			{
				pushInt(popInt() ? 0 : 1);
				break;
			}
			case OBJ_OP_LOGICGE:
			{
				popInt2(&left, &right);
				pushInt(left >= right ? 1 : 0);
				break;
			}
			case OBJ_OP_LOGICGT:
			{
				popInt2(&left, &right);
				pushInt(left > right ? 1 : 0);
				break;
			}
			case OBJ_OP_LOGICLE:
			{
				popInt2(&left, &right);
				pushInt(left <= right ? 1 : 0);
				break;
			}
			case OBJ_OP_LOGICLT:
			{
				popInt2(&left, &right);
				pushInt(left < right ? 1 : 0);
				break;
			}
			case OBJ_OP_LOGICEQU:
			{
				popInt2(&left, &right);
				pushInt(left == right ? 1 : 0);
				break;
			}
			case OBJ_OP_LOGICNE:
			{
				popInt2(&left, &right);
				pushInt(left != right ? 1 : 0);
				break;
			}
			case OBJ_FUNC_LOWLIMIT:
			{
				popInt2(&left, &right);

				if (left >= right)
					pushInt(left);
				else
					Error("Expression \"%s\" at offset %d in section \"%s\" out of range", makePatchString(patch, section), patch->offset, section->name);

				break;
			}
			case OBJ_FUNC_HIGHLIMIT:
			{
				popInt2(&left, &right);

				if (left <= right)
					pushInt(left);
				else
					Error("Expression \"%s\" at offset %d in section \"%s\" out of range", makePatchString(patch, section), patch->offset, section->name);

				break;
			}
			case OBJ_FUNC_FDIV:
			{
				popInt2(&left, &right);
				pushInt(imuldiv(left, 65536, right));
				break;
			}
			case OBJ_FUNC_FMUL:
			{
				popInt2(&left, &right);
				pushInt(imuldiv(left, right, 65536));
				break;
			}
			case OBJ_FUNC_ATAN2:
			{
				popInt2(&left, &right);
				pushInt(fatan2(left, right));
				break;
			}
			case OBJ_FUNC_SIN:
			{
				pushInt(fsin(popInt()));
				break;
			}
			case OBJ_FUNC_COS:
			{
				pushInt(fcos(popInt()));
				break;
			}
			case OBJ_FUNC_TAN:
			{
				pushInt(ftan(popInt()));
				break;
			}
			case OBJ_FUNC_ASIN:
			{
				pushInt(fasin(popInt()));
				break;
			}
			case OBJ_FUNC_ACOS:
			{
				pushInt(facos(popInt()));
				break;
			}
			case OBJ_FUNC_ATAN:
			{
				pushInt(fatan(popInt()));
				break;
			}
			case OBJ_CONSTANT:
			{
				uint32_t value;

				value  = (*expression++);
				value |= (*expression++) << 8;
				value |= (*expression++) << 16;
				value |= (*expression++) << 24;

				pushInt(value);

				size -= 4;
				break;
			}
			case OBJ_SYMBOL:
			{
				uint32_t symbolId;

				symbolId  = (*expression++);
				symbolId |= (*expression++) << 8;
				symbolId |= (*expression++) << 16;
				symbolId |= (*expression++) << 24;

				pushInt(sect_GetSymbolValue(section, symbolId));
				size -= 4;
				break;
			}
			case OBJ_FUNC_BANK:
			{
				uint32_t symbolId;

				symbolId  = (*expression++);
				symbolId |= (*expression++) << 8;
				symbolId |= (*expression++) << 16;
				symbolId |= (*expression++) << 24;

				pushInt(sect_GetSymbolBank(section, symbolId));
				size -= 4;
				break;
			}
			case OBJ_PCREL:
			{
				popInt2(&left, &right);
				pushInt(left + right - (patch->offset + section->imageLocation));
				break;
			}
			default:
				Error("Unknown patch operator");
				break;
		}
	}

	return popInt();
}

static void patchSection(Section* section)
{
	Patches* patches = section->patches;

	if (patches != NULL)
	{
		Patch* patch = patches->patches;
		uint32_t i;

		for (i = patches->totalPatches; i > 0; --i, ++patch)
		{
			int32_t value = calculatePatchValue(patch, section);

			switch (patch->type)
			{
				case PATCH_BYTE:
				{
					if (value >= -128 && value <= 255)
						section->data[patch->offset] = (uint8_t)value;
					else
						Error("Expression \"%s\" at offset %d in section \"%s\" out of range", makePatchString(patch, section), patch->offset, section->name);
					break;
				}
				case PATCH_LWORD:
				{
					if (value >= -32768 && value <= 65535)
					{
						section->data[patch->offset + 0] = (uint8_t)value;
						section->data[patch->offset + 1] = (uint8_t)(value >> 8);
					}
					else
					{
						Error("patch out of range");
					}
					break;
				}
				case PATCH_BWORD:
				{
					if (value>=-32768 && value<=65535)
					{
						section->data[patch->offset+0] = (uint8_t)(value>>8);
						section->data[patch->offset+1] = (uint8_t)value;
					}
					else
					{
						Error("patch out of range");
					}
					break;
				}
				case PATCH_LLONG:
				{
					section->data[patch->offset+0] = (uint8_t)value;
					section->data[patch->offset+1] = (uint8_t)(value>>8);
					section->data[patch->offset+2] = (uint8_t)(value>>16);
					section->data[patch->offset+3] = (uint8_t)(value>>24);
					break;
				}
				case PATCH_BLONG:
				{
					section->data[patch->offset+0] = (uint8_t)(value>>24);
					section->data[patch->offset+1] = (uint8_t)(value>>16);
					section->data[patch->offset+2] = (uint8_t)(value>>8);
					section->data[patch->offset+3] = (uint8_t)value;
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


void patch_Process(void)
{
	Section* section = g_sections;

	while (section != NULL)
	{
		if (section->used)
			patchSection(section);

		section = section->nextSection;
	}
}


Patches* patch_Alloc(uint32_t totalPatches)
{
	Patches* patches = mem_Alloc(sizeof(Patches) + totalPatches * sizeof(Patch));
	if (patches != NULL)
	{
		patches->totalPatches = totalPatches;
	}

	return patches;
}

