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

/*	char	ID[4]="XOB\1";
 *	char	MinimumWordSize ; Used for address calculations.
 *							; 1 - A CPU address points to a byte in memory
 *							; 2 - A CPU address points to a 16 bit word in memory (CPU address 0x1000 is the 0x2000th byte)
 *							; 4 - A CPU address points to a 32 bit word in memory (CPU address 0x1000 is the 0x4000th byte)
 *	uint32_t	NumberOfGroups
 *	REPT	NumberOfGroups
 *			ASCIIZ	Name
 *			uint32_t	Type
 *	ENDR
 *	uint32_t	NumberOfSections
 *	REPT	NumberOfSections
 *			int32_t	GroupID	; -1 = exported EQU symbols
 *			ASCIIZ	Name
 *			int32_t	Bank	; -1 = not bankfixed
 *			int32_t	Position; -1 = not fixed
 *			int32_t	BasePC	; -1 = not fixed
 *			uint32_t	NumberOfSymbols
 *			REPT	NumberOfSymbols
 *					ASCIIZ	Name
 *					uint32_t	Type	;0=EXPORT
 *									;1=IMPORT
 *									;2=LOCAL
 *									;3=LOCALEXPORT
 *									;4=LOCALIMPORT
 *					IF Type==EXPORT or LOCAL or LOCALEXPORT
 *						int32_t	value
 *					ENDC
 *			ENDR
 *			uint32_t	Size
 *			IF	SectionCanContainData
 *					uint8_t	Data[Size]
 *					uint32_t	NumberOfPatches
 *					REPT	NumberOfPatches
 *							uint32_t	Offset
 *							uint32_t	Type
 *							uint32_t	ExprSize
 *							uint8_t	Expr[ExprSize]
 *					ENDR
 *			ENDC
 *	ENDR
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "asmotor.h"
#include "xasm.h"
#include "section.h"
#include "expression.h"
#include "symbol.h"
#include "tokens.h"
#include "object.h"
#include "patch.h"
#include "file.h"


/* Private functions */

static uint32_t
writeSymbols(SSection* section, FILE* fileHandle, SExpression* expression, uint32_t nextId) {
    if (expression != NULL) {
        nextId = writeSymbols(section, fileHandle, expression->left, nextId);
        nextId = writeSymbols(section, fileHandle, expression->right, nextId);

        if (expr_Type(expression) == EXPR_SYMBOL || (g_pConfiguration->bSupportBanks && expr_IsOperator(expression, T_FUNC_BANK))) {
            if (expression->value.symbol->ID == UINT32_MAX) {
                expression->value.symbol->ID = nextId++;
                fputsz(str_String(expression->value.symbol->pName), fileHandle);
                if (expression->value.symbol->pSection == section) {
                    if (expression->value.symbol->nFlags & SYMF_LOCALEXPORT) {
                        fputll(3, fileHandle);    //	LOCALEXPORT
                        fputll((uint32_t) expression->value.symbol->Value.Value, fileHandle);
                    } else if (expression->value.symbol->nFlags & SYMF_EXPORT) {
                        fputll(0, fileHandle);    //	EXPORT
                        fputll((uint32_t) expression->value.symbol->Value.Value, fileHandle);
                    } else {
                        fputll(2, fileHandle);    //	LOCAL
                        fputll((uint32_t) expression->value.symbol->Value.Value, fileHandle);
                    }
                } else if (expression->value.symbol->eType == SYM_IMPORT || expression->value.symbol->eType == SYM_GLOBAL) {
                    fputll(1, fileHandle);    //	IMPORT
                } else {
                    fputll(4, fileHandle);    //	LOCALIMPORT
                }
            }
        }
    }
    return nextId;
}

static uint32_t
writeExportedSymbols(FILE* fileHandle, SSection* section, uint32_t symbolId) {
    for (uint_fast16_t i = 0; i < HASHSIZE; ++i) {
        for (SSymbol* sym = g_pHashedSymbols[i]; sym; sym = list_GetNext(sym)) {
            if (sym->eType != SYM_GROUP)
                sym->ID = (uint32_t) -1;

            if (sym->pSection == section && (sym->nFlags & (SYMF_EXPORT | SYMF_LOCALEXPORT))) {
                sym->ID = symbolId++;

                fputsz(str_String(sym->pName), fileHandle);
                if (sym->nFlags & SYMF_EXPORT)
                    fputll(0, fileHandle);    //	EXPORT
                else if (sym->nFlags & SYMF_LOCALEXPORT)
                    fputll(3, fileHandle);    //	LOCALEXPORT
                fputll((uint32_t) sym->Value.Value, fileHandle);
            }
        }
    }

    return symbolId;
}

static void
markLocalExportsInExpression(SSection* section, SExpression* expression) {
    if (expression != NULL) {
        markLocalExportsInExpression(section, expression->left);
        markLocalExportsInExpression(section, expression->right);

        if ((expr_Type(expression) == EXPR_SYMBOL || (g_pConfiguration->bSupportBanks && expr_IsOperator(expression, T_FUNC_BANK)))
            && (expression->value.symbol->nFlags & SYMF_EXPORTABLE) && expression->value.symbol->pSection != section) {
            expression->value.symbol->nFlags |= SYMF_LOCALEXPORT;
        }
    }
}

static void
markLocalExports() {
    for (SSection* section = g_pSectionList; section; section = list_GetNext(section)) {
        for (SPatch* patch = section->patches; patch; patch = list_GetNext(patch)) {
            if (patch->section == section) {
                markLocalExportsInExpression(section, patch->expression);
            }
        }
    }
}

static void
writeExpression(FILE* fileHandle, SExpression* expression) {
    if (expression != NULL) {
        writeExpression(fileHandle, expression->left);
        writeExpression(fileHandle, expression->right);

        switch (expr_Type(expression)) {
            case EXPR_PARENS: {
                writeExpression(fileHandle, expression->right);
                break;
            }
            case EXPR_OPERATION: {
                switch (expression->operation) {
                    default:
                        internalerror("Unknown operator");
                        break;
                    case T_OP_SUBTRACT:
                        fputc(OBJ_OP_SUB, fileHandle);
                        break;
                    case T_OP_ADD:
                        fputc(OBJ_OP_ADD, fileHandle);
                        break;
                    case T_OP_BITWISE_XOR:
                        fputc(OBJ_OP_XOR, fileHandle);
                        break;
                    case T_OP_BITWISE_OR:
                        fputc(OBJ_OP_OR, fileHandle);
                        break;
                    case T_OP_BITWISE_AND:
                        fputc(OBJ_OP_AND, fileHandle);
                        break;
                    case T_OP_BITWISE_ASL:
                        fputc(OBJ_OP_ASL, fileHandle);
                        break;
                    case T_OP_BITWISE_ASR:
                        fputc(OBJ_OP_ASR, fileHandle);
                        break;
                    case T_OP_MULTIPLY:
                        fputc(OBJ_OP_MUL, fileHandle);
                        break;
                    case T_OP_DIVIDE:
                        fputc(OBJ_OP_DIV, fileHandle);
                        break;
                    case T_OP_MODULO:
                        fputc(OBJ_OP_MOD, fileHandle);
                        break;
                    case T_OP_BOOLEAN_OR:
                        fputc(OBJ_OP_BOOLEAN_OR, fileHandle);
                        break;
                    case T_OP_BOOLEAN_AND:
                        fputc(OBJ_OP_BOOLEAN_AND, fileHandle);
                        break;
                    case T_OP_BOOLEAN_NOT:
                        fputc(OBJ_OP_BOOLEAN_NOT, fileHandle);
                        break;
                    case T_OP_GREATER_OR_EQUAL:
                        fputc(OBJ_OP_GREATER_OR_EQUAL, fileHandle);
                        break;
                    case T_OP_GREATER_THAN:
                        fputc(OBJ_OP_GREATER_THAN, fileHandle);
                        break;
                    case T_OP_LESS_OR_EQUAL:
                        fputc(OBJ_OP_LESS_OR_EQUAL, fileHandle);
                        break;
                    case T_OP_LESS_THAN:
                        fputc(OBJ_OP_LESS_THAN, fileHandle);
                        break;
                    case T_OP_EQUAL:
                        fputc(OBJ_OP_EQUALS, fileHandle);
                        break;
                    case T_OP_NOT_EQUAL:
                        fputc(OBJ_OP_NOT_EQUALS, fileHandle);
                        break;
                    case T_FUNC_LOWLIMIT:
                        fputc(OBJ_FUNC_LOW_LIMIT, fileHandle);
                        break;
                    case T_FUNC_HIGHLIMIT:
                        fputc(OBJ_FUNC_HIGH_LIMIT, fileHandle);
                        break;
                    case T_FUNC_FDIV:
                        fputc(OBJ_FUNC_FDIV, fileHandle);
                        break;
                    case T_FUNC_FMUL:
                        fputc(OBJ_FUNC_FMUL, fileHandle);
                        break;
                    case T_FUNC_ATAN2:
                        fputc(OBJ_FUNC_ATAN2, fileHandle);
                        break;
                    case T_FUNC_SIN:
                        fputc(OBJ_FUNC_SIN, fileHandle);
                        break;
                    case T_FUNC_COS:
                        fputc(OBJ_FUNC_COS, fileHandle);
                        break;
                    case T_FUNC_TAN:
                        fputc(OBJ_FUNC_TAN, fileHandle);
                        break;
                    case T_FUNC_ASIN:
                        fputc(OBJ_FUNC_ASIN, fileHandle);
                        break;
                    case T_FUNC_ACOS:
                        fputc(OBJ_FUNC_ACOS, fileHandle);
                        break;
                    case T_FUNC_ATAN:
                        fputc(OBJ_FUNC_ATAN, fileHandle);
                        break;
                    case T_FUNC_BANK: {
                        assert (g_pConfiguration->bSupportBanks);
                        fputc(OBJ_FUNC_BANK, fileHandle);
                        fputll(expression->value.symbol->ID, fileHandle);
                        break;
                    }
                }

                break;
            }
            case EXPR_CONSTANT: {
                fputc(OBJ_CONSTANT, fileHandle);
                fputll((uint32_t) expression->value.integer, fileHandle);
                break;
            }
            case EXPR_SYMBOL: {
                fputc(OBJ_SYMBOL, fileHandle);
                fputll(expression->value.symbol->ID, fileHandle);
                break;
            }
            case EXPR_PC_RELATIVE: {
                fputc(OBJ_PC_REL, fileHandle);
                break;
            }
            default: {
                internalerror("Unknown expression");
                break;
            }
        }
    }
}

static void
writePatch(FILE* fileHandle, SPatch* patch) {
    fputll(patch->offset, fileHandle);
    fputll(patch->type, fileHandle);
    off_t sizePosition = ftello(fileHandle);
    fputll(0, fileHandle);

    off_t expressionStart = ftello(fileHandle);
    writeExpression(fileHandle, patch->expression);
    off_t expressionEnd = ftell(fileHandle);

    fseek(fileHandle, sizePosition, SEEK_SET);
    fputll((uint32_t) (expressionEnd - expressionStart), fileHandle);
    fseek(fileHandle, expressionEnd, SEEK_SET);
}

static void
writeGroups(FILE* fileHandle) {
    off_t sizePos = ftello(fileHandle);
    fputll(0, fileHandle);

    uint32_t groupCount = 0;
    for (uint_fast16_t i = 0; i < HASHSIZE; ++i) {
        for (SSymbol* sym = g_pHashedSymbols[i]; sym != NULL; sym = list_GetNext(sym)) {
            if (sym->eType == SYM_GROUP) {
                sym->ID = groupCount++;
                fputsz(str_String(sym->pName), fileHandle);
                fputll(sym->Value.GroupType | (sym->nFlags & (SYMF_CHIP | SYMF_DATA)), fileHandle);
            }
        }
    }

    fseeko(fileHandle, sizePos, SEEK_SET);
    fputll(groupCount, fileHandle);

    fseeko(fileHandle, 0, SEEK_END);
}

static void
writeExportedConstantsSection(FILE* fileHandle) {
    fputll(UINT32_MAX, fileHandle);  //	GroupID , -1 for EQU symbols
    fputc(0, fileHandle);            //	Name
    fputll(UINT32_MAX, fileHandle);  //	Bank
    fputll(UINT32_MAX, fileHandle);  //	Org
    fputll(UINT32_MAX, fileHandle);  //	BasePC

    off_t symbolCountPos = ftell(fileHandle);
    fputll(0, fileHandle);        //	Number of symbols
    uint32_t integerExportCount = 0;

    for (uint_fast16_t i = 0; i < HASHSIZE; ++i) {
        for (SSymbol* sym = g_pHashedSymbols[i]; sym; sym = list_GetNext(sym)) {
            if ((sym->eType == SYM_EQU || sym->eType == SYM_SET) && (sym->nFlags & SYMF_EXPORT)) {
                ++integerExportCount;
                fputsz(str_String(sym->pName), fileHandle);
                fputll(0, fileHandle);    /* EXPORT */
                fputll((uint32_t) sym->Value.Value, fileHandle);
            }
        }
    }

    fseek(fileHandle, symbolCountPos, SEEK_SET);
    fputll(integerExportCount, fileHandle);
    fseek(fileHandle, 0, SEEK_END);

    fputll(0, fileHandle); // Size
}

static void
writeSectionSymbols(FILE* fileHandle, SSection* section) {
    off_t symbolCountPosition = ftello(fileHandle);
    fputll(0, fileHandle);

    uint32_t symbolId = writeExportedSymbols(fileHandle, section, 0);

    // Calculate and export symbols IDs by going through patches
    for (SPatch* patch = section->patches; patch; patch = list_GetNext(patch)) {
        if (patch->section == section) {
            symbolId = writeSymbols(section, fileHandle, patch->expression, symbolId);
        }
    }

    // Fix up number of symbols
    fseeko(fileHandle, symbolCountPosition, SEEK_SET);
    fputll(symbolId, fileHandle);
    fseeko(fileHandle, 0, SEEK_END);
}

static void
writeSectionPatches(FILE* fileHandle, SSection* section) {
    off_t patchCountPos = ftell(fileHandle);
    fputll(0, fileHandle);

    uint32_t totalPatches = 0;
    for (SPatch* patch = section->patches; patch; patch = list_GetNext(patch)) {
        if (patch->section == section) {
            writePatch(fileHandle, patch);
            totalPatches += 1;
        }
    }

    fseek(fileHandle, patchCountPos, SEEK_SET);
    fputll(totalPatches, fileHandle);
    fseek(fileHandle, 0, SEEK_END);
}

static void
writeSection(FILE* fileHandle, SSection* section) {
    fputll(section->group->ID, fileHandle);
    fputsz(str_String(section->name), fileHandle);
    if (section->flags & SECTF_BANKFIXED) {
        assert(g_pConfiguration->bSupportBanks);
        fputll(section->bank, fileHandle);
    } else {
        fputll(UINT32_MAX, fileHandle);
    }

    fputll(section->flags & SECTF_LOADFIXED ? section->imagePosition : UINT32_MAX, fileHandle);
    fputll(section->flags & SECTF_LOADFIXED ? section->cpuOrigin : UINT32_MAX, fileHandle);

    writeSectionSymbols(fileHandle, section);

    fputll(section->usedSpace, fileHandle);
    if (section->group->Value.GroupType == GROUP_TEXT) {
        fwrite(section->data, 1, section->usedSpace, fileHandle);
        writeSectionPatches(fileHandle, section);
    }
}


/* Public functions */

bool
obj_Write(string* pName) {
    FILE* fileHandle;
    if ((fileHandle = fopen(str_String(pName), "wb")) == NULL)
        return false;

    fwrite("XOB\1", 1, 4, fileHandle);
    fputc(g_pConfiguration->eMinimumWordSize, fileHandle);

    writeGroups(fileHandle);

    //	Output sections

    uint32_t sectionCount = sect_TotalSections();
    fputll(sectionCount + 1, fileHandle);

    writeExportedConstantsSection(fileHandle);

    markLocalExports();

    for (SSection* section = g_pSectionList; section; section = list_GetNext(section)) {
        writeSection(fileHandle, section);
    }

    fclose(fileHandle);
    return true;
}
