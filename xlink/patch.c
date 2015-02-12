/*  Copyright 2008-2015 Carsten Elton Sorensen

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


typedef struct StackEntry_
{
	Section* section;
	int32_t value;
} StackEntry;


static char* s_stringStack[STACKSIZE];
static StackEntry s_stack[STACKSIZE];
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


static void	popString2(char** outLeft, char** outRight)
{
	*outRight = popString();
	*outLeft = popString();
}


static void pushSectionInt(Section* section, int32_t value)
{
	if (s_stackIndex >= STACKSIZE)
		Error("patch too complex");

	StackEntry entry = { section, value };
	s_stack[s_stackIndex++] = entry;
}


static void pushInt(int32_t value)
{
	pushSectionInt(NULL, value);
}


static StackEntry popInt(void)
{
	if (s_stackIndex > 0)
		return s_stack[--s_stackIndex];

	Error("mangled patch");
	return s_stack[0]; // dummy return
}


static void	popInt2(StackEntry* outLeft, StackEntry* outRight)
{
	*outRight = popInt();
	*outLeft = popInt();
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

		switch ((ExpressionOperator)*expression++)
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

#define combine_operator(left, right, operator) \
	popInt2(&left, &right); \
	if (left.section == NULL && right.section == NULL) { \
		pushInt(left.value operator right.value); \
	} else { \
		Error("Expression \"%s\" at offset %d in section \"%s\" attempts to combine two values from different sections", makePatchString(patch, section), patch->offset, section->name); \
	}

#define combine_func(left, right, operator) \
	popInt2(&left, &right); \
	if (left.section == NULL && right.section == NULL) { \
		pushInt(operator(left.value, right.value)); \
	} else { \
		Error("Expression \"%s\" at offset %d in section \"%s\" attempts to combine two values from different sections", makePatchString(patch, section), patch->offset, section->name); \
	}

#define unary(left, operator) \
	left = popInt(); \
	if (left.section == NULL) { \
		pushSectionInt(left.section, operator(left.value)); \
	} else { \
		Error("Expression \"%s\" at offset %d in section \"%s\" attempts to perform a unary operation on a value relative to a section", makePatchString(patch, section), patch->offset, section->name); \
	}


bool_t calculatePatchValue(Patch* patch, Section* section, int32_t* outValue, Section** outSection)
{
	int32_t size = patch->expressionSize;
	uint8_t* expression = patch->expression;

	s_stackIndex = 0;

	while (size > 0)
	{
		StackEntry left, right;

		--size;

		switch ((ExpressionOperator)*expression++)
		{
			case OBJ_OP_SUB:
			{
				popInt2(&left, &right);
				if (left.section == right.section || (left.section != NULL && right.section == NULL))
					pushSectionInt(left.section, left.value - right.value);
				else
					Error("Expression \"%s\" at offset %d in section \"%s\" attempts to subtract two values from different sections", makePatchString(patch, section), patch->offset, section->name);
				break;
			}
			case OBJ_OP_ADD:
			{
				popInt2(&left, &right);
				if (right.section == NULL)
					pushSectionInt(left.section, left.value + right.value);
				else if (left.section == NULL)
					pushSectionInt(right.section, left.value + right.value);
				else
					Error("Expression \"%s\" at offset %d in section \"%s\" attempts to add two values from different sections", makePatchString(patch, section), patch->offset, section->name);
				break;
			}
			case OBJ_OP_XOR:
			{
				combine_operator(left, right, ^)
				break;
			}
			case OBJ_OP_OR:
			{
				combine_operator(left, right, |)
				break;
			}
			case OBJ_OP_AND:
			{
				combine_operator(left, right, &)
				break;
			}
			case OBJ_OP_SHL:
			{
				combine_operator(left, right, <<)
				break;
			}
			case OBJ_OP_SHR:
			{
				combine_operator(left, right, >>)
				break;
			}
			case OBJ_OP_MUL:
			{
				combine_operator(left, right, *)
				break;
			}
			case OBJ_OP_DIV:
			{
				combine_operator(left, right, /)
				break;
			}
			case OBJ_OP_MOD:
			{
				combine_operator(left, right, %)
				break;
			}
			case OBJ_OP_LOGICOR:
			{
				combine_operator(left, right, ||)
				break;
			}
			case OBJ_OP_LOGICAND:
			{
				combine_operator(left, right, &&)
				break;
			}
			case OBJ_OP_LOGICGE:
			{
				combine_operator(left, right, >=)
				break;
			}
			case OBJ_OP_LOGICGT:
			{
				combine_operator(left, right, >)
				break;
			}
			case OBJ_OP_LOGICLE:
			{
				combine_operator(left, right, <=)
				break;
			}
			case OBJ_OP_LOGICLT:
			{
				combine_operator(left, right, <)
				break;
			}
			case OBJ_OP_LOGICEQU:
			{
				combine_operator(left, right, ==)
				break;
			}
			case OBJ_OP_LOGICNE:
			{
				combine_operator(left, right, !=)
				break;
			}
			case OBJ_OP_LOGICNOT:
			{
				unary(left, !)
				break;
			}
			case OBJ_FUNC_SIN:
			{
				unary(left, fsin)
				break;
			}
			case OBJ_FUNC_COS:
			{
				unary(left, fcos)
				break;
			}
			case OBJ_FUNC_TAN:
			{
				unary(left, ftan)
				break;
			}
			case OBJ_FUNC_ASIN:
			{
				unary(left, fasin)
				break;
			}
			case OBJ_FUNC_ACOS:
			{
				unary(left, facos)
				break;
			}
			case OBJ_FUNC_ATAN:
			{
				unary(left, fatan)
				break;
			}
			case OBJ_FUNC_LOWLIMIT:
			{
				popInt2(&left, &right);

				if (left.section == NULL && right.section == NULL && left.value >= right.value)
					pushInt(left.value);
				else
					Error("Expression \"%s\" at offset %d in section \"%s\" out of range", makePatchString(patch, section), patch->offset, section->name);

				break;
			}
			case OBJ_FUNC_HIGHLIMIT:
			{
				popInt2(&left, &right);

				if (left.section == NULL && right.section == NULL && left.value <= right.value)
					pushInt(left.value);
				else
					Error("Expression \"%s\" at offset %d in section \"%s\" out of range", makePatchString(patch, section), patch->offset, section->name);

				break;
			}
			case OBJ_FUNC_FDIV:
			{
				combine_func(left, right, fdiv)
				break;
			}
			case OBJ_FUNC_FMUL:
			{
				combine_func(left, right, fmul)
				break;
			}
			case OBJ_FUNC_ATAN2:
			{
				combine_func(left, right, fatan2)
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
				int32_t symbolValue;
				Section* symbolSection;

				symbolId  = (*expression++);
				symbolId |= (*expression++) << 8;
				symbolId |= (*expression++) << 16;
				symbolId |= (*expression++) << 24;

				if (!sect_GetSymbolValue(section, symbolId, &symbolValue, &symbolSection))
					return false;

				pushSectionInt(symbolSection, symbolValue);
				size -= 4;
				break;
			}
			case OBJ_FUNC_BANK:
			{
				uint32_t symbolId;
				int32_t bank;

				symbolId  = (*expression++);
				symbolId |= (*expression++) << 8;
				symbolId |= (*expression++) << 16;
				symbolId |= (*expression++) << 24;

				if (!sect_GetConstantSymbolBank(section, symbolId, &bank))
					return false;

				pushInt(bank);
				size -= 4;
				break;
			}
			case OBJ_PCREL:
			{
				combine_operator(left, right, +)
				pushSectionInt(section, patch->offset);
				combine_operator(left, right, -)
				break;
			}
			default:
			{
				Error("Unknown patch operator");
				break;
			}
		}
	}

	StackEntry entry = popInt();
	*outValue = entry.value;
	*outSection = entry.section;
	return s_stackIndex == 0;
}


static void patchSection(Section* section, bool_t allowReloc)
{
	Patches* patches = section->patches;

	if (patches != NULL)
	{
		Patch* patch = patches->patches;
		uint32_t i;

		for (i = patches->totalPatches; i > 0; --i, ++patch)
		{
			int32_t value;

			if (calculatePatchValue(patch, section, &value, &patch->offsetSection))
			{
				switch (patch->type)
				{
					case PATCH_BYTE:
					{
						if (patch->offsetSection == NULL && value >= -128 && value <= 255)
							section->data[patch->offset] = (uint8_t)value;
						else
							Error("Expression \"%s\" at offset %d in section \"%s\" out of range", makePatchString(patch, section), patch->offset, section->name);

						break;
					}
					case PATCH_LWORD:
					{
						if (patch->offsetSection == NULL && value >= -32768 && value <= 65535)
						{
							section->data[patch->offset + 0] = (uint8_t)value;
							section->data[patch->offset + 1] = (uint8_t)(value >> 8);
						}
						else
						{
							Error("Expression \"%s\" at offset %d in section \"%s\" out of range", makePatchString(patch, section), patch->offset, section->name);
						}
						break;
					}
					case PATCH_BWORD:
					{
						if (patch->offsetSection == NULL && value >= -32768 && value <= 65535)
						{
							section->data[patch->offset + 0] = (uint8_t)(value >> 8);
							section->data[patch->offset + 1] = (uint8_t)value;
						}
						else
						{
							Error("Expression \"%s\" at offset %d in section \"%s\" out of range", makePatchString(patch, section), patch->offset, section->name);
						}
						break;
					}
					case PATCH_LLONG:
					{
						section->data[patch->offset + 0] = (uint8_t)value;
						section->data[patch->offset + 1] = (uint8_t)(value >> 8);
						section->data[patch->offset + 2] = (uint8_t)(value >> 16);
						section->data[patch->offset + 3] = (uint8_t)(value >> 24);
						break;
					}
					case PATCH_BLONG:
					{
						section->data[patch->offset + 0] = (uint8_t)(value >> 24);
						section->data[patch->offset + 1] = (uint8_t)(value >> 16);
						section->data[patch->offset + 2] = (uint8_t)(value >> 8);
						section->data[patch->offset + 3] = (uint8_t)value;
						break;
					}
					case PATCH_NONE:
					case PATCH_RELOC:
					{
						break;
					}
					default:
					{
						Error("unhandled patch type");
						break;
					}
				}
				mem_Free(patch->expression);

				if (patch->offsetSection && !allowReloc)
				{
					Error("Expression \"%s\" at offset %d in section \"%s\" is relocatable", makePatchString(patch, section), patch->offset, section->name);
					return;
				}
				else if (allowReloc)
				{
					patch->type = PATCH_RELOC;
				}
				else
				{
					patch->type = PATCH_NONE;
					patch->offset = 0;
					patch->offsetSection = NULL;
				}
				patch->expression = NULL;
				patch->expressionSize = 0;
			}
		}
	}
}


void patch_Process(bool_t allowReloc)
{
    for (Section* section = g_sections; section != NULL; section = section->nextSection)
    {
        if (section->used)
			patchSection(section, allowReloc);
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

