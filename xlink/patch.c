/*  Copyright 2008-2017 Carsten Elton Sorensen

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

#include <string.h>

#include "fmath.h"
#include "mem.h"
#include "str.h"

#include "asmotor.h"

#include "patch.h"
#include "section.h"
#include "symbol.h"
#include "xlink.h"


#define STACKSIZE 256

typedef struct StackEntry_ {
    SSymbol* symbol;
    int32_t value;
} StackEntry;

static char* g_stringStack[STACKSIZE];
static StackEntry g_stack[STACKSIZE];
static int32_t g_stackIndex;

static void
pushString(char* stringValue) {
    if (g_stackIndex >= STACKSIZE)
        error("patch too complex");

    g_stringStack[g_stackIndex++] = stringValue;
}

static void
pushStringCopy(const char* stringValue) {
    size_t len = strlen(stringValue) + 1;
    char* copy = mem_Alloc(len);
    strncpy(copy, stringValue, len);

    pushString(copy);
}

static void
pushIntAsString(uint32_t intValue) {
    char* value = mem_Alloc(16);
    snprintf(value, 16, "%u", intValue);

    pushString(value);
}

static char*
popString(void) {
    if (g_stackIndex > 0)
        return g_stringStack[--g_stackIndex];

    error("mangled patch");
    return NULL;
}

static void
popStringPair(char** outLeft, char** outRight) {
    *outRight = popString();
    *outLeft = popString();
}

static void
pushSymbolInt(SSymbol* symbol, int32_t value) {
    if (g_stackIndex >= STACKSIZE)
        error("patch too complex");

    StackEntry entry = {symbol, value};
    g_stack[g_stackIndex++] = entry;
}

static void
pushInt(int32_t value) {
    pushSymbolInt(NULL, value);
}

static StackEntry
popInt(void) {
    if (g_stackIndex > 0)
        return g_stack[--g_stackIndex];

    error("mangled patch");
    return g_stack[0]; // dummy return
}

static void
popIntPair(StackEntry* outLeft, StackEntry* outRight) {
    *outRight = popInt();
    *outLeft = popInt();
}

static void
combinePatchStrings(char* operator) {
    char* left;
    char* right;
    char* result;

    popStringPair(&left, &right);

    size_t sz = strlen(left) + strlen(right) + strlen(operator) + 5;
    result = (char*) mem_Alloc(sz);
    snprintf(result, sz, "(%s)%s(%s)", left, operator, right);

    pushString(result);

    mem_Free(left);
    mem_Free(right);
}

static void
combinePatchFunctionStrings(char* function) {
    char* left;
    char* right;
    char* result;

    popStringPair(&left, &right);

    size_t sz = strlen(left) + strlen(right) + strlen(function) + 4;
    result = (char*) mem_Alloc(sz);
    snprintf(result, sz, "%s(%s,%s)", function, left, right);

    pushString(result);

    mem_Free(left);
    mem_Free(right);
}

static void
combinePatchFunctionString(char* function) {
    char* left;
    char* result;

    left = popString();

    size_t sz = strlen(left) + strlen(function) + 3;
    result = (char*) mem_Alloc(sz);
    snprintf(result, sz, "%s(%s)", function, left);

    pushString(result);

    mem_Free(left);
}

static char*
makePatchString(SPatch* patch, SSection* section) {
    int32_t size = patch->expressionSize;
    uint8_t* expression = patch->expression;

    g_stackIndex = 0;

    while (size-- > 0) {
        char* left;
        char* right;

        switch ((EExpressionOperator) *expression++) {
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
            case OBJ_OP_ASL:
                combinePatchStrings("<<");
                break;
            case OBJ_OP_ASR:
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
            case OBJ_OP_BOOLEAN_OR:
                combinePatchStrings("||");
                break;
            case OBJ_OP_BOOLEAN_AND:
                combinePatchStrings("&&");
                break;
            case OBJ_OP_BOOLEAN_NOT:
                combinePatchFunctionString("!");
                break;
            case OBJ_OP_GREATER_OR_EQUAL:
                combinePatchStrings(">=");
                break;
            case OBJ_OP_GREATER_THAN:
                combinePatchStrings(">");
                break;
            case OBJ_OP_LESS_OR_EQUAL:
                combinePatchStrings("<=");
                break;
            case OBJ_OP_LESS_THAN:
                combinePatchStrings("<");
                break;
            case OBJ_OP_EQUALS:
                combinePatchStrings("==");
                break;
            case OBJ_OP_NOT_EQUALS:
                combinePatchStrings("!=");
                break;
            case OBJ_FUNC_LOW_LIMIT: {
                popStringPair(&left, &right);
                pushString(left);
                mem_Free(right);
                break;
            }
            case OBJ_FUNC_HIGH_LIMIT: {
                popStringPair(&left, &right);
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
            case OBJ_CONSTANT: {
                uint32_t value;

                value = (*expression++);
                value |= (*expression++) << 8u;
                value |= (*expression++) << 16u;
                value |= (*expression++) << 24u;

                pushIntAsString(value);

                size -= 4;
                break;
            }
            case OBJ_SYMBOL: {
                uint32_t symbolId;

                symbolId = (*expression++);
                symbolId |= (*expression++) << 8u;
                symbolId |= (*expression++) << 16u;
                symbolId |= (*expression++) << 24u;

                pushStringCopy(sect_GetSymbolName(section, symbolId));

                size -= 4;
                break;
            }
            case OBJ_FUNC_BANK: {
                char* symbolName;
                char* copy;
                uint32_t symbolId;

                symbolId = (*expression++);
                symbolId |= (*expression++) << 8u;
                symbolId |= (*expression++) << 16u;
                symbolId |= (*expression++) << 24u;

                symbolName = sect_GetSymbolName(section, symbolId);

                size_t sz = strlen(symbolName) + 7;
                copy = mem_Alloc(sz);
                snprintf(copy, sz, "BANK(%s)", symbolName);

                pushString(copy);

                size -= 4; 
                break;
            }
            case OBJ_PC_REL: {
                combinePatchStrings("+");
                pushStringCopy("*");
                combinePatchStrings("-");
                break;
            }
            default:
                error("Unknown patch operator");
                break;
        }
    }

    return popString();
}

#define combine_bitwise(left, right, operator) \
    popIntPair(&(left), &(right)); \
    if ((left).symbol == NULL && (right).symbol == NULL) { \
        pushInt((uint32_t)(left).value operator (uint32_t)(right).value); \
    } else { \
        error("Expression \"%s\" at offset %d in section \"%s\" attempts to combine two values from different sections", makePatchString(patch, section), patch->offset, section->name); \
    }

#define combine_operator(left, right, operator) \
    popIntPair(&(left), &(right)); \
    if ((left).symbol == NULL && (right).symbol == NULL) { \
        pushInt((left).value operator (right).value); \
    } else { \
        error("Expression \"%s\" at offset %d in section \"%s\" attempts to combine two values from different sections", makePatchString(patch, section), patch->offset, section->name); \
    }

#define combine_func(left, right, operator) \
    popIntPair(&(left), &(right)); \
    if ((left).symbol == NULL && (right).symbol == NULL) { \
        pushInt(operator((left).value, (right).value)); \
    } else { \
        error("Expression \"%s\" at offset %d in section \"%s\" attempts to combine two values from different sections", makePatchString(patch, section), patch->offset, section->name); \
    }

#define unary(left, operator) \
    (left) = popInt(); \
    if ((left).symbol == NULL) { \
        pushSymbolInt((left).symbol, operator((left).value)); \
    } else { \
        error("Expression \"%s\" at offset %d in section \"%s\" attempts to perform a unary operation on a value relative to a section", makePatchString(patch, section), patch->offset, section->name); \
    }

static bool
calculatePatchValue(SPatch* patch, SSection* section, bool allowImports, int32_t* outValue, SSymbol** outSymbol) {
    int32_t size = patch->expressionSize;
    uint8_t* expression = patch->expression;

    g_stackIndex = 0;

    while (size > 0) {
        StackEntry left, right;

        --size;

        switch ((EExpressionOperator) *expression++) {
            case OBJ_OP_SUB: {
                popIntPair(&left, &right);
                if (left.symbol == NULL && right.symbol == NULL)
                    pushInt(left.value - right.value);
                else if (left.symbol != NULL && right.symbol == NULL)
                    pushSymbolInt(left.symbol, left.value - right.value);
                else if (left.symbol != NULL && right.symbol != NULL && left.symbol->section == right.symbol->section)
                    pushInt(left.symbol->value - right.symbol->value);
                else
                    error("Expression \"%s\" at offset %d in section \"%s\" attempts to subtract two values from different sections",
                          makePatchString(patch, section), patch->offset, section->name);
                break;
            }
            case OBJ_OP_ADD: {
                popIntPair(&left, &right);
                if (left.symbol == NULL && right.symbol == NULL)
                    pushInt(left.value + right.value);
                else if (right.symbol == NULL)
                    pushSymbolInt(left.symbol, left.value + right.value);
                else if (left.symbol == NULL)
                    pushSymbolInt(right.symbol, left.value + right.value);
                else
                    error("Expression \"%s\" at offset %d in section \"%s\" attempts to add two values from different sections",
                          makePatchString(patch, section), patch->offset, section->name);
                break;
            }
            case OBJ_OP_XOR: {
                combine_bitwise(left, right, ^)
                break;
            }
            case OBJ_OP_OR: {
                combine_bitwise(left, right, |)
                break;
            }
            case OBJ_OP_AND: {
                combine_bitwise(left, right, &)
                break;
            }
            case OBJ_OP_ASL: {
                combine_bitwise(left, right, <<)
                break;
            }
            case OBJ_OP_ASR: {
                combine_bitwise(left, right, >>)
                break;
            }
            case OBJ_OP_MUL: {
                combine_operator(left, right, *)
                break;
            }
            case OBJ_OP_DIV: {
                combine_operator(left, right, /)
                break;
            }
            case OBJ_OP_MOD: {
                combine_operator(left, right, %)
                break;
            }
            case OBJ_OP_BOOLEAN_OR: {
                combine_operator(left, right, ||)
                break;
            }
            case OBJ_OP_BOOLEAN_AND: {
                combine_operator(left, right, &&)
                break;
            }
            case OBJ_OP_GREATER_OR_EQUAL: {
                combine_operator(left, right, >=)
                break;
            }
            case OBJ_OP_GREATER_THAN: {
                combine_operator(left, right, >)
                break;
            }
            case OBJ_OP_LESS_OR_EQUAL: {
                combine_operator(left, right, <=)
                break;
            }
            case OBJ_OP_LESS_THAN: {
                combine_operator(left, right, <)
                break;
            }
            case OBJ_OP_EQUALS: {
                combine_operator(left, right, ==)
                break;
            }
            case OBJ_OP_NOT_EQUALS: {
                combine_operator(left, right, !=)
                break;
            }
            case OBJ_OP_BOOLEAN_NOT: {
                unary(left, !)
                break;
            }
            case OBJ_FUNC_SIN: {
                unary(left, fsin)
                break;
            }
            case OBJ_FUNC_COS: {
                unary(left, fcos)
                break;
            }
            case OBJ_FUNC_TAN: {
                unary(left, ftan)
                break;
            }
            case OBJ_FUNC_ASIN: {
                unary(left, fasin)
                break;
            }
            case OBJ_FUNC_ACOS: {
                unary(left, facos)
                break;
            }
            case OBJ_FUNC_ATAN: {
                unary(left, fatan)
                break;
            }
            case OBJ_FUNC_LOW_LIMIT: {
                popIntPair(&left, &right);

                if (left.symbol == NULL && right.symbol == NULL && left.value >= right.value)
                    pushInt(left.value);
                else
                    error("Expression \"%s\" at offset %d in section \"%s\" out of range (%d must be >= %d)",
                          makePatchString(patch, section), patch->offset, section->name, left.value, right.value);

                break;
            }
            case OBJ_FUNC_HIGH_LIMIT: {
                popIntPair(&left, &right);

                if (left.symbol == NULL && right.symbol == NULL && left.value <= right.value)
                    pushInt(left.value);
                else
                    error("Expression \"%s\" at offset %d in section \"%s\" out of range (%d must be <= %d)",
                          makePatchString(patch, section), patch->offset, section->name, left.value, right.value);

                break;
            }
            case OBJ_FUNC_FDIV: {
                combine_func(left, right, fdiv)
                break;
            }
            case OBJ_FUNC_FMUL: {
                combine_func(left, right, fmul)
                break;
            }
            case OBJ_FUNC_ATAN2: {
                combine_func(left, right, fatan2)
                break;
            }
            case OBJ_CONSTANT: {
                uint32_t value;

                value = (*expression++);
                value |= (*expression++) << 8u;
                value |= (*expression++) << 16u;
                value |= (*expression++) << 24u;

                pushInt(value);

                size -= 4;
                break;
            }
            case OBJ_SYMBOL: {
                uint32_t symbolId;
                SSymbol* symbol;

                symbolId = (*expression++);
                symbolId |= (*expression++) << 8u;
                symbolId |= (*expression++) << 16u;
                symbolId |= (*expression++) << 24u;

                symbol = sect_GetSymbol(section, symbolId, allowImports);
                if (symbol->section != NULL && symbol->section->cpuLocation != -1)
                    pushSymbolInt(NULL, symbol->value);
                else
                    pushSymbolInt(symbol, 0);
                size -= 4;
                break;
            }
            case OBJ_FUNC_BANK: {
                uint32_t symbolId;
                int32_t bank;

                symbolId = (*expression++);
                symbolId |= (*expression++) << 8u;
                symbolId |= (*expression++) << 16u;
                symbolId |= (*expression++) << 24u;

                if (!sect_GetConstantSymbolBank(section, symbolId, &bank))
                    return false;

                pushInt(bank);
                size -= 4;
                break;
            }
            case OBJ_PC_REL: {
                combine_operator(left, right, +)
                left = popInt();
                if (left.symbol == NULL)
                    pushInt(left.value - (section->cpuLocation + patch->offset));
                else if (left.symbol->section == section)
                    pushInt(left.symbol->value + left.value - patch->offset);
                else
                    error("Illegal PC relative expression \"%s\" at offset %d in section \"%s\" attempts to subtract two values from different sections",
                          makePatchString(patch, section), patch->offset, section->name);
                break;
            }
            default: {
                error("Unknown patch operator");
                break;
            }
        }
    }

    StackEntry entry = popInt();
    *outValue = entry.value;
    *outSymbol = entry.symbol;
    return g_stackIndex == 0;
}

static void
patchSection(SSection* section, bool allowReloc, bool onlySectionRelativeReloc, bool allowImports) {
    SPatches* patches = section->patches;

    if (patches != NULL) {
        SPatch* patch = patches->patches;

        for (uint32_t i = patches->totalPatches; i > 0; --i, ++patch) {
            SSymbol* valueSymbol;
            int32_t value;

            if (calculatePatchValue(patch, section, allowImports, &value, &valueSymbol)) {
                if (valueSymbol != NULL) {
                    if (!allowReloc) {
                        error("Expression \"%s\" at offset %d in section \"%s\" is relocatable",
                              makePatchString(patch, section), patch->offset, section->name);
                        return;
                    } else if (onlySectionRelativeReloc || symbol_IsLocal(valueSymbol)) {
                        value += valueSymbol->value;
                        patch->valueSection = valueSymbol->section;
                        patch->valueSymbol = NULL;
                    } else {
                        patch->valueSection = NULL;
                        patch->valueSymbol = valueSymbol;

                    }
                }

                switch (patch->type) {
                    case PATCH_8: {
                        if (valueSymbol == NULL && value >= -128 && value <= 255)
                            section->data[patch->offset] = (uint8_t) value;
                        else
                            error("Expression \"%s\" at offset %d in section \"%s\" out of range",
                                  makePatchString(patch, section), patch->offset, section->name);

                        break;
                    }
                    case PATCH_LE_16: {
                        if (valueSymbol == NULL && value >= -32768 && value <= 65535) {
                            section->data[patch->offset + 0] = (uint8_t) value;
                            section->data[patch->offset + 1] = (uint8_t) ((uint32_t) value >> 8u);
                        } else {
                            error("Expression \"%s\" at offset %d in section \"%s\" out of range",
                                  makePatchString(patch, section), patch->offset, section->name);
                        }
                        break;
                    }
                    case PATCH_BE_16: {
                        if (valueSymbol == NULL && value >= -32768 && value <= 65535) {
                            section->data[patch->offset + 0] = (uint8_t) ((uint32_t) value >> 8u);
                            section->data[patch->offset + 1] = (uint8_t) value;
                        } else {
                            error("Expression \"%s\" at offset %d in section \"%s\" out of range",
                                  makePatchString(patch, section), patch->offset, section->name);
                        }
                        break;
                    }
                    case PATCH_LE_32: {
                        section->data[patch->offset + 0] = (uint8_t) value;
                        section->data[patch->offset + 1] = (uint8_t) ((uint32_t) value >> 8u);
                        section->data[patch->offset + 2] = (uint8_t) ((uint32_t) value >> 16u);
                        section->data[patch->offset + 3] = (uint8_t) ((uint32_t) value >> 24u);
                        break;
                    }
                    case PATCH_BE_32: {
                        section->data[patch->offset + 0] = (uint8_t) ((uint32_t) value >> 24u);
                        section->data[patch->offset + 1] = (uint8_t) ((uint32_t) value >> 16u);
                        section->data[patch->offset + 2] = (uint8_t) ((uint32_t) value >> 8u);
                        section->data[patch->offset + 3] = (uint8_t) value;
                        break;
                    }
                    case PATCH_NONE:
                    case PATCH_RELOC: {
                        break;
                    }
                    default: {
                        error("unhandled patch type");
                        break;
                    }
                }
                mem_Free(patch->expression);

                if (allowReloc) {
                    patch->type = PATCH_RELOC;
                } else {
                    patch->type = PATCH_NONE;
                    patch->offset = 0;
                    patch->valueSymbol = NULL;
                    patch->valueSection = NULL;
                }
                patch->expression = NULL;
                patch->expressionSize = 0;
            }
        }
    }
}

extern void
patch_Process(bool allowReloc, bool onlySectionRelativeReloc, bool allowImports) {
    for (SSection* section = sect_Sections; section != NULL; section = section->nextSection) {
        if (section->used)
            patchSection(section, allowReloc, onlySectionRelativeReloc, allowImports);
    }
}

extern SPatches*
patch_Alloc(uint32_t totalPatches) {
    SPatches* patches = mem_Alloc(sizeof(SPatches) + totalPatches * sizeof(SPatch));
    if (patches != NULL) {
        patches->totalPatches = totalPatches;
    }

    return patches;
}

