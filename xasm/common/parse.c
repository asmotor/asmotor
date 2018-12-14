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

#include <fmath.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "asmotor.h"
#include "types.h"
#include "mem.h"
#include "expression.h"
#include "project.h"
#include "filestack.h"
#include "lexer.h"
#include "parse.h"
#include "symbol.h"
#include "section.h"
#include "tokens.h"
#include "options.h"
#include "parse_string.h"

#define REPT_LEN 4
#define ENDR_LEN 4
#define ENDC_LEN 4
#define MACRO_LEN 5
#define ENDM_LEN 4

extern bool
parse_TargetSpecific(void);

/* Private functions */

static bool
isWhiteSpace(char s) {
    return s == ' ' || s == '\t' || s == '\0' || s == '\n';
}

static bool
isToken(size_t index, const char* token) {
    size_t len = strlen(token);
    return lex_CompareNoCase(index, token, len) && isWhiteSpace(lex_PeekChar(index + len));
}

static bool
isRept(size_t index) {
    return isToken(index, "REPT");
}

static bool
isEndr(size_t index) {
    return isToken(index, "ENDR");
}

static bool
isIf(size_t index) {
    return isToken(index, "IF") || isToken(index, "IFC") || isToken(index, "IFD") || isToken(index, "IFNC")
           || isToken(index, "IFND") || isToken(index, "IFNE") || isToken(index, "IFEQ") || isToken(index, "IFGT")
           || isToken(index, "IFGE") || isToken(index, "IFLT") || isToken(index, "IFLE");
}

static bool
isElse(size_t index) {
    return isToken(index, "ELSE");
}

static bool
isEndc(size_t index) {
    return isToken(index, "ENDC");
}

static bool
isMacro(size_t index) {
    return isToken(index, "MACRO");
}

static bool
isEndm(size_t index) {
    return isToken(index, "ENDM");
}

static size_t
skipLine(size_t index) {
    while (lex_PeekChar(index) != 0) {
        if (lex_PeekChar(index++) == '\n')
            return index;
    }

    return SIZE_MAX;
}

static size_t
findControlToken(size_t index) {
    if (index == SIZE_MAX)
        return SIZE_MAX;

    if (isRept(index) || isEndr(index) || isIf(index) || isElse(index) || isEndc(index) || isMacro(index)
        || isEndm(index)) {
        return index;
    }

    while (!isWhiteSpace(lex_PeekChar(index))) {
        if (lex_PeekChar(index++) == ':')
            break;
    }
    while (isWhiteSpace(lex_PeekChar(index)))
        ++index;

    return index;
}

static size_t
getReptBodySize(size_t index);

static size_t
getIfBodySize(size_t index);

static size_t
getMacroBodySize(size_t index);

static bool
skipRept(size_t* index) {
    if (isRept(*index)) {
		*index = skipLine(*index + getReptBodySize(*index + REPT_LEN) + REPT_LEN + ENDR_LEN);
		return true;
	} else {
		return false;
	}
}

static bool
skipIf(size_t* index) {
	if (isIf(*index)) {
		while (!isWhiteSpace(lex_PeekChar(*index)))
			*index += 1;
		*index = skipLine(*index + getIfBodySize(*index) + ENDC_LEN);
		return true;
	} else {
		return false;
	}
}

static bool
skipMacro(size_t* index) {
    if (isMacro(*index)) {
		*index = skipLine(*index + getMacroBodySize(*index + MACRO_LEN) + MACRO_LEN + ENDM_LEN);
		return true;
	} else {
		return false;
	}
}

static size_t
getBlockBodySize(size_t index, bool (*endPredicate)(size_t)) {
    size_t start = index;

    index = skipLine(index);
    while ((index = findControlToken(index)) != SIZE_MAX) {
        if (!skipRept(&index) && !skipIf(&index) && !skipMacro(&index)) {
            if (endPredicate(index)) {
                return index - start;
            } else {
                index = skipLine(index);
            }
        }
    }

    return 0;
}

static size_t
getReptBodySize(size_t index) {
    return getBlockBodySize(index, isEndr);
}

static size_t
getMacroBodySize(size_t index) {
    return getBlockBodySize(index, isEndm);
}

static size_t
getIfBodySize(size_t index) {
    return getBlockBodySize(index, isEndc);
}

static bool
copyReptBlock(char** reptBlock, size_t* size) {
    size_t len = getReptBodySize(0);

    if (len == 0)
        return false;

    *size = len;

    *reptBlock = (char*) mem_Alloc(len + 1);
    (*reptBlock)[len] = 0;
    lex_GetChars(*reptBlock, len);
    fstk_Current->lineNumber += lex_SkipBytes(4);

    return true;
}

static bool
skipToElse(void) {
    size_t index = skipLine(0);
    while ((index = findControlToken(index)) != SIZE_MAX) {
        if (!skipRept(&index) && !skipIf(&index) && !skipMacro(&index)) {
			if (isEndc(index)) {
				fstk_Current->lineNumber += lex_SkipBytes(index) + 1;
				return true;
			} else if (isElse(index)) {
				fstk_Current->lineNumber += lex_SkipBytes(index + 4) + 1;
				return true;
			} else {
				index = skipLine(index);
			}
		}
    }

    return false;
}

static bool
skipToEndc(void) {
    size_t index = skipLine(0);
    while ((index = findControlToken(index)) != SIZE_MAX) {
        if (!skipRept(&index) && !skipIf(&index) && !skipMacro(&index)) {
			if (isEndc(index)) {
				fstk_Current->lineNumber += lex_SkipBytes(index);
				return true;
			} else {
				index = skipLine(index);
			}
		}
    }

    return 0;
}

static bool
copyMacroBlock(char** dest, size_t* size) {
    size_t len = getMacroBodySize(0);

    *size = len;

    *dest = (char*) mem_Alloc(len + 1);
    fstk_Current->lineNumber += lex_GetChars(*dest, len);
    fstk_Current->lineNumber += lex_SkipBytes(4);
    return true;
}

static uint32_t
colonCount(void) {
    if (lex_Current.token == ':') {
        parse_GetToken();
        if (lex_Current.token == ':') {
            parse_GetToken();
            return 2;
        }
    }
    return 1;
}

static void
parse_RS(string* pName, int32_t size, int coloncount) {
    string* pRS = str_Create("__RS");
    int32_t nRS = sym_GetValueByName(pRS);

    sym_CreateSET(pName, nRS);
    sym_CreateSET(pRS, nRS + size);

    str_Free(pRS);

    if (coloncount == 2)
        sym_Export(pName);
}

static void
parse_RS_Skip(int32_t size) {
    string* pRS = str_Create("__RS");
    sym_CreateSET(pRS, sym_GetValueByName(pRS) + size);
    str_Free(pRS);
}

static bool
parse_Symbol(void) {
    bool r = false;

    if (lex_Current.token == T_LABEL) {
        string* symbolName = str_Create(lex_Current.value.string);

        parse_GetToken();
        uint32_t totalColons = colonCount();

        switch (lex_Current.token) {
            default: {
                sym_CreateLabel(symbolName);
                if (totalColons == 2)
                    sym_Export(symbolName);
                r = true;
                break;
            }
            case T_POP_RB: {
                parse_GetToken();
                parse_RS(symbolName, parse_ConstantExpression(), totalColons);
                r = true;
                break;
            }
            case T_POP_RW: {
                parse_GetToken();
                parse_RS(symbolName, parse_ConstantExpression() * 2, totalColons);
                r = true;
                break;
            }
            case T_POP_RL: {
                parse_GetToken();
                parse_RS(symbolName, parse_ConstantExpression() * 4, totalColons);
                r = true;
                break;
            }
            case T_POP_EQU: {
                parse_GetToken();
                sym_CreateEQU(symbolName, parse_ConstantExpression());
                if (totalColons == 2)
                    sym_Export(symbolName);
                r = true;
                break;
            }
            case T_POP_SET: {
                parse_GetToken();
                sym_CreateSET(symbolName, parse_ConstantExpression());
                if (totalColons == 2)
                    sym_Export(symbolName);
                r = true;
                break;
            }
            case T_POP_EQUS: {
                parse_GetToken();

                string* value = parse_ExpectStringExpression();
                if (value != NULL) {
                    sym_CreateEQUS(symbolName, value);
                    str_Free(value);
                    if (totalColons == 2)
                        sym_Export(symbolName);
                    r = true;
                }
                break;
            }
            case T_POP_GROUP: {
                parse_GetToken();
                switch (lex_Current.token) {
                    case T_GROUP_TEXT:
                        sym_CreateGROUP(symbolName, GROUP_TEXT);
                        r = true;
                        break;
                    case T_GROUP_BSS:
                        sym_CreateGROUP(symbolName, GROUP_BSS);
                        r = true;
                        break;
                    default:
                        prj_Error(ERROR_EXPECT_TEXT_BSS);
                        r = false;
                        break;
                }
                if (totalColons == 2)
                    sym_Export(symbolName);
                break;
            }
            case T_POP_MACRO: {
                uint32_t lineNumber = fstk_Current->lineNumber;

                size_t reptSize;
                char* reptBlock;
                if (copyMacroBlock(&reptBlock, &reptSize)) {
                    sym_CreateMACRO(symbolName, reptBlock, reptSize);
                    parse_GetToken();
                    r = true;
                } else {
                    prj_Fail(ERROR_NEED_ENDM, str_String(fstk_Current->name), lineNumber);
                }
                break;
            }
        }
        str_Free(symbolName);
    }

    return r;
}

static bool
parse_Import(string* pName) {
    return sym_Import(pName) != NULL;
}

static bool
parse_Export(string* pName) {
    return sym_Export(pName) != NULL;
}

static bool
parse_Global(string* pName) {
    return sym_Global(pName) != NULL;
}

static bool
parse_SymbolOp(bool (* pOp)(string* pName)) {
    bool r = false;

    parse_GetToken();
    if (lex_Current.token == T_ID) {
        string* pName = str_Create(lex_Current.value.string);
        pOp(pName);
        str_Free(pName);
        parse_GetToken();
        while (lex_Current.token == ',') {
            parse_GetToken();
            if (lex_Current.token == T_ID) {
                pName = str_Create(lex_Current.value.string);
                pOp(pName);
                str_Free(pName);
                r = true;
                parse_GetToken();
            } else
                prj_Error(ERROR_EXPECT_IDENTIFIER);
        }
    } else
        prj_Error(ERROR_EXPECT_IDENTIFIER);

    return r;
}

static int32_t
parse_ExpectBankFixed(void) {
    int32_t bank;

    if (!g_pConfiguration->bSupportBanks)
        internalerror("Banks not supported");

    if (lex_Current.token != T_FUNC_BANK) {
        prj_Error(ERROR_EXPECT_BANK);
        return -1;
    }

    parse_GetToken();
    if (!parse_ExpectChar('['))
        return -1;

    parse_GetToken();
    bank = parse_ConstantExpression();
    if (!parse_ExpectChar(']'))
        return -1;

    return bank;
}

static bool
parse_PseudoOp(void) {
    switch (lex_Current.token) {
        case T_POP_REXIT: {
            if (fstk_Current->type != CONTEXT_REPT) {
                prj_Warn(WARN_REXIT_OUTSIDE_REPT);
            } else {
                fstk_Current->block.repeat.remaining = 0;
                fstk_ProcessNextBuffer();
                fstk_Current->lineNumber++;
            }
            parse_GetToken();
            return true;
        }
        case T_POP_MEXIT: {
            if (fstk_Current->type != CONTEXT_MACRO) {
                prj_Warn(WARN_MEXIT_OUTSIDE_MACRO);
            } else {
                fstk_ProcessNextBuffer();
            }
            parse_GetToken();
            return true;
        }
        case T_POP_SECTION: {
            parse_GetToken();
            string* name = parse_ExpectStringExpression();
            if (name == NULL)
                return true;

            if (!parse_ExpectChar(','))
                return sect_SwitchTo_NAMEONLY(name);

            if (lex_Current.token != T_ID) {
                prj_Error(ERROR_EXPECT_IDENTIFIER);
                return false;
            }

            string* pGroup = str_Create(lex_Current.value.string);
            SSymbol* sym = sym_FindSymbol(pGroup);
            str_Free(pGroup);

            if (sym->eType != SYM_GROUP) {
                prj_Error(ERROR_IDENTIFIER_GROUP);
                return true;
            }
            parse_GetToken();

            if (g_pConfiguration->bSupportBanks && lex_Current.token == ',') {
                int32_t bank;
                parse_GetToken();
                bank = parse_ExpectBankFixed();

                if (bank == -1)
                    return true;

                return sect_SwitchTo_BANK(name, sym, bank);
            } else if (lex_Current.token != '[') {
                return sect_SwitchTo(name, sym);
            }
            parse_GetToken();

            int32_t loadAddress;

            loadAddress = parse_ConstantExpression();
            if (!parse_ExpectChar(']'))
                return true;

            if (g_pConfiguration->bSupportBanks && lex_Current.token == ',') {
                int32_t bank;
                parse_GetToken();
                bank = parse_ExpectBankFixed();

                if (bank == -1)
                    return true;

                return sect_SwitchTo_LOAD_BANK(name, sym, loadAddress, bank);
            }

            return sect_SwitchTo_LOAD(name, sym, loadAddress);
        }
        case T_POP_ORG: {
            parse_GetToken();

            uint32_t orgAddress = (uint32_t) parse_ConstantExpression();
            sect_SetOrgAddress(orgAddress);

            return true;
        }
        case T_POP_PRINTT: {
            parse_GetToken();

            string* r = parse_ExpectStringExpression();
            if (r != NULL) {
                printf("%s", str_String(r));
                str_Free(r);
                return true;
            }

            return false;
        }
        case T_POP_PRINTV: {
            parse_GetToken();
            printf("$%X", parse_ConstantExpression());
            return true;
        }
        case T_POP_PRINTF: {
            parse_GetToken();

            int32_t i = parse_ConstantExpression();
            if (i < 0) {
                printf("-");
                i = -i;
            }
            printf("%d.%05d", (uint32_t) i >> 16u, imuldiv((uint32_t) i & 0xFFFFu, 100000, 65536));

            return true;
        }
        case T_POP_IMPORT: {
            return parse_SymbolOp(parse_Import);
        }
        case T_POP_EXPORT: {
            return parse_SymbolOp(parse_Export);
        }
        case T_POP_GLOBAL: {
            return parse_SymbolOp(parse_Global);
        }
        case T_POP_PURGE: {
            tokens_expandStrings = false;
            bool r = parse_SymbolOp(sym_Purge);
            tokens_expandStrings = true;

            return r;
        }
        case T_POP_RSRESET: {
            string* pRS = str_Create("__RS");
            parse_GetToken();
            sym_CreateSET(pRS, 0);
            str_Free(pRS);
            return true;
        }
        case T_POP_RSSET: {
            string* pRS = str_Create("__RS");
            parse_GetToken();
            int32_t val = parse_ConstantExpression();
            sym_CreateSET(pRS, val);
            str_Free(pRS);
            return true;
        }
        case T_POP_RB: {
            parse_RS_Skip(parse_ConstantExpression());
            return true;
        }
        case T_POP_RW: {
            parse_RS_Skip(parse_ConstantExpression() * 2);
            return true;
        }
        case T_POP_RL: {
            parse_RS_Skip(parse_ConstantExpression() * 4);
            return true;
        }
        case T_POP_FAIL: {
            parse_GetToken();

            string* r = parse_ExpectStringExpression();
            if (r != NULL) {
                prj_Fail(WARN_USER_GENERIC, str_String(r));
                return true;
            } else {
                internalerror("String expression is NULL");
            }
        }
        case T_POP_WARN: {
            parse_GetToken();

            string* r = parse_ExpectStringExpression();
            if (r != NULL) {
                prj_Warn(WARN_USER_GENERIC, str_String(r));
                return true;
            } else {
                internalerror("String expression is NULL");
            }
        }
        case T_POP_EVEN: {
            parse_GetToken();
            sect_Align(2);
            return true;
        }
        case T_POP_CNOP: {
            parse_GetToken();

            int32_t offset = parse_ConstantExpression();
            if (offset < 0) {
                prj_Error(ERROR_EXPR_POSITIVE);
                return true;
            }

            if (!parse_ExpectComma())
                return true;

            int32_t align = parse_ConstantExpression();
            if (align < 0) {
                prj_Error(ERROR_EXPR_POSITIVE);
                return true;
            }
            sect_Align((uint32_t) align);
            sect_SkipBytes((uint32_t) offset);
            return true;
        }
        case T_POP_DSB: {
            parse_GetToken();

            int32_t offset = parse_ConstantExpression();
            if (offset >= 0) {
                sect_SkipBytes((uint32_t) offset);
                return true;
            } else {
                prj_Error(ERROR_EXPR_POSITIVE);
                return true;
            }
        }
        case T_POP_DSW: {
            parse_GetToken();

            int32_t offset = parse_ConstantExpression();
            if (offset >= 0) {
                sect_SkipBytes((uint32_t) offset * 2);
                return true;
            } else {
                prj_Error(ERROR_EXPR_POSITIVE);
                return true;
            }
        }
        case T_POP_DSL: {
            parse_GetToken();

            int32_t offset = parse_ConstantExpression();
            if (offset >= 0) {
                sect_SkipBytes((uint32_t) offset * 4);
                return true;
            } else {
                prj_Error(ERROR_EXPR_POSITIVE);
                return true;
            }
        }
        case T_POP_DB: {
            do {
                parse_GetToken();

                SExpression* expr;
                string* str;

                if ((expr = parse_Expression(1)) != NULL) {
                    expr = expr_CheckRange(expr, -128, 255);
                    if (expr != NULL) {
                        sect_OutputExpr8(expr);
                    } else {
                        prj_Error(ERROR_EXPRESSION_N_BIT, 8);
                    }
                } else if ((str = parse_StringExpression()) != NULL) {
                    const char* s = str_String(str);
                    while (*s) {
                        sect_OutputConst8((uint8_t) *s++);
                    }

                    str_Free(str);
                } else {
                    sect_SkipBytes(1); //prj_Error(ERROR_INVALID_EXPRESSION);
                }
            } while (lex_Current.token == ',');

            return true;
        }
        case T_POP_DW: {
            do {
                parse_GetToken();
                SExpression* expr = parse_Expression(2);
                if (expr != NULL) {
                    expr = expr_CheckRange(expr, -32768, 65535);
                    if (expr != NULL) {
                        sect_OutputExpr16(expr);
                    } else {
                        prj_Error(ERROR_EXPRESSION_N_BIT, 16);
                    }
                } else {
                    sect_SkipBytes(2); // prj_Error(ERROR_INVALID_EXPRESSION);
                }
            } while (lex_Current.token == ',');

            return true;
        }
        case T_POP_DL: {
            do {
                parse_GetToken();

                SExpression* expr = parse_Expression(4);
                if (expr != NULL) {
                    sect_OutputExpr32(expr);
                } else {
                    sect_SkipBytes(4); //prj_Error(ERROR_INVALID_EXPRESSION);
                }
            } while (lex_Current.token == ',');

            return true;
        }
        case T_POP_INCLUDE: {
            SLexerBookmark mark;
            lex_Bookmark(&mark);

            parse_GetToken();

            string* filename = parse_StringExpression();

            if (filename == NULL) {
                size_t endIndex = 0;

                while (lex_PeekChar(endIndex) == ' ' || lex_PeekChar(endIndex) == '\t')
                    ++endIndex;

                while (!isspace((unsigned char) lex_PeekChar(endIndex)))
                    ++endIndex;

                filename = lex_GetString(endIndex);
                parse_GetToken();
            }

            if (filename != NULL) {
                fstk_ProcessIncludeFile(filename);
                str_Free(filename);
                return true;
            } else {
                prj_Error(ERROR_EXPR_STRING);
                return false;
            }
        }
        case T_POP_INCBIN: {
            parse_GetToken();

            string* filename = parse_ExpectStringExpression();
            if (filename != NULL) {
                sect_OutputBinaryFile(filename);
                str_Free(filename);
                return true;
            }
            return false;
        }
        case T_POP_REPT: {
            parse_GetToken();
            int32_t reptCount = parse_ConstantExpression();

            size_t reptSize;
            char* reptBlock;
            if (copyReptBlock(&reptBlock, &reptSize)) {
                if (reptCount > 0) {
                    fstk_ProcessRepeatBlock(reptBlock, reptSize, (uint32_t) reptCount);
                } else if (reptCount < 0) {
                    prj_Error(ERROR_EXPR_POSITIVE);
                    mem_Free(reptBlock);
                } else {
                    mem_Free(reptBlock);
                }
                return true;
            } else {
                prj_Fail(ERROR_NEED_ENDR);
                return false;
            }
        }
        case T_POP_SHIFT: {
            parse_GetToken();

            SExpression* expr = parse_Expression(4);
            if (expr != NULL) {
                if (expr_IsConstant(expr)) {
                    fstk_ShiftMacroArgs(expr->value.integer);
                    expr_Free(expr);
                    return true;
                } else {
                    prj_Fail(ERROR_EXPR_CONST);
                    return false;
                }
            } else {
                fstk_ShiftMacroArgs(1);
                return true;
            }
        }
        case T_POP_IFC: {
            parse_GetToken();

            string * s1 = parse_ExpectStringExpression();
            if (s1 != NULL) {
                if (parse_ExpectComma()) {
                    string* s2 = parse_ExpectStringExpression();
                    if (s2 != NULL) {
                        if (!str_Equal(s1, s2)) {
                            skipToElse();
                        }

                        str_Free(s1);
                        str_Free(s2);
                        return true;
                    } else {
                        prj_Error(ERROR_EXPR_STRING);
                    }
                }
                str_Free(s1);
            } else {
                prj_Error(ERROR_EXPR_STRING);
            }

            return false;
        }
        case T_POP_IFNC: {
            parse_GetToken();

            string* s1 = parse_ExpectStringExpression();
            if (s1 != NULL) {
                if (parse_ExpectComma()) {
                    string* s2 = parse_ExpectStringExpression();
                    if (s2 != NULL) {
                        if (str_Equal(s1, s2)) {
                            skipToElse();
                        }

                        str_Free(s1);
                        str_Free(s2);
                        return true;
                    } else {
                        prj_Error(ERROR_EXPR_STRING);
                    }
                }
                mem_Free(s1);
            } else {
                prj_Error(ERROR_EXPR_STRING);
            }

            return false;
        }
        case T_POP_IFD: {
            parse_GetToken();

            if (lex_Current.token == T_ID) {
                string* pName = str_Create(lex_Current.value.string);
                if (sym_IsDefined(pName)) {
                    parse_GetToken();
                } else {
                    parse_GetToken();
                    /* will continue parsing just after ELSE or just at ENDC keyword */
                    skipToElse();
                }
                str_Free(pName);
                return true;
            }
            prj_Error(ERROR_EXPECT_IDENTIFIER);
            return false;
        }
        case T_POP_IFND: {
            parse_GetToken();

            if (lex_Current.token == T_ID) {
                string* pName = str_Create(lex_Current.value.string);
                if (!sym_IsDefined(pName)) {
                    parse_GetToken();
                } else {
                    parse_GetToken();
                    /* will continue parsing just after ELSE or just at ENDC keyword */
                    skipToElse();
                }
                str_Free(pName);
                return true;
            }
            prj_Error(ERROR_EXPECT_IDENTIFIER);
            return false;
        }
        case T_POP_IF: {
            parse_GetToken();

            if (parse_ConstantExpression() == 0) {
                /* will continue parsing just after ELSE or just at ENDC keyword */
                skipToElse();
                parse_GetToken();
            }
            return true;
        }
        case T_POP_IFEQ: {
            parse_GetToken();

            if (parse_ConstantExpression() != 0) {
                /* will continue parsing just after ELSE or just at ENDC keyword */
                skipToElse();
            }
            return true;
        }
        case T_POP_IFGT: {
            parse_GetToken();

            if (parse_ConstantExpression() <= 0) {
                /* will continue parsing just after ELSE or just at ENDC keyword */
                skipToElse();
            }
            return true;
        }
        case T_POP_IFGE: {
            parse_GetToken();

            if (parse_ConstantExpression() < 0) {
                /* will continue parsing just after ELSE or just at ENDC keyword */
                skipToElse();
            }
            return true;
        }
        case T_POP_IFLT: {
            parse_GetToken();

            if (parse_ConstantExpression() >= 0) {
                /* will continue parsing just after ELSE or just at ENDC keyword */
                skipToElse();
            }
            return true;
        }
        case T_POP_IFLE: {
            parse_GetToken();

            if (parse_ConstantExpression() > 0) {
                /* will continue parsing just after ELSE or just at ENDC keyword */
                skipToElse();
            }
            return true;
        }
        case T_POP_ELSE: {
            /* will continue parsing just at ENDC keyword */
            skipToEndc();
            parse_GetToken();
            return true;
        }
        case T_POP_ENDC: {
            parse_GetToken();
            return true;
        }
        case T_POP_PUSHO: {
            opt_Push();
            parse_GetToken();
            return true;
        }
        case T_POP_POPO: {
            opt_Pop();
            parse_GetToken();
            return true;
        }
        case T_POP_OPT: {
            lex_SetState(LEX_STATE_MACRO_ARGS);
            parse_GetToken();
            if (lex_Current.token == T_STRING) {
                opt_Parse(lex_Current.value.string);
                parse_GetToken();
                while (lex_Current.token == ',') {
                    parse_GetToken();
                    opt_Parse(lex_Current.value.string);
                    parse_GetToken();
                }
            }
            lex_SetState(LEX_STATE_NORMAL);
            return true;
        }
        default: {
            return false;
        }
    }
}

/* Public functions */

bool
parse_IsDot(SLexerBookmark* pBookmark) {
    if (pBookmark)
        lex_Bookmark(pBookmark);

    if (lex_Current.token == '.') {
        parse_GetToken();
        return true;
    }

    if (lex_Current.token == T_ID && lex_Current.value.string[0] == '.') {
        lex_UnputString(lex_Current.value.string + 1);
        parse_GetToken();
        return true;
    }

    return false;
}

bool
parse_ExpectChar(char ch) {
    if (lex_Current.token == (uint32_t) ch) {
        parse_GetToken();
        return true;
    } else {
        prj_Error(ERROR_CHAR_EXPECTED, ch);
        return false;
    }
}

static bool
parse_Misc(void) {
    switch (lex_Current.token) {
        case T_ID: {
            string* pName = str_Create(lex_Current.value.string);
            bool bIsMacro = sym_IsMacro(pName);
            str_Free(pName);

            if (bIsMacro) {
                string* s = str_Create(lex_Current.value.string);

                lex_SetState(LEX_STATE_MACRO_ARG0);
                parse_GetToken();
                while (lex_Current.token != '\n') {
                    if (lex_Current.token == T_STRING) {
                        fstk_AddMacroArgument(lex_Current.value.string);
                        parse_GetToken();
                        if (lex_Current.token == ',') {
                            parse_GetToken();
                        } else if (lex_Current.token != '\n') {
                            prj_Error(ERROR_CHAR_EXPECTED, ',');
                            lex_SetState(LEX_STATE_NORMAL);
                            parse_GetToken();
                            return false;
                        }
                    } else if (lex_Current.token == T_MACROARG0) {
                        fstk_SetMacroArgument0(lex_Current.value.string);
                        parse_GetToken();
                    } else {
                        internalerror("Must be T_STRING");
                    }
                }
                lex_SetState(LEX_STATE_NORMAL);
                fstk_ProcessMacro(s);
                str_Free(s);
                return true;
            } else {
                prj_Error(ERROR_INSTR_UNKNOWN, lex_Current.value.string);
                return false;
            }
        }
        default: {
            return false;
        }
    }
}


void
parse_GetToken(void) {
    if (lex_GetNextToken()) {
        return;
    }

    prj_Fail(ERROR_END_OF_FILE);
}


bool
parse_Do(void) {
    bool r = true;

    lex_GetNextToken();

    while (lex_Current.token && r) {
        if (!parse_TargetSpecific() && !parse_Symbol() && !parse_PseudoOp() && !parse_Misc()) {
            if (lex_Current.token == '\n') {
                lex_GetNextToken();
                fstk_Current->lineNumber += 1;
                g_nTotalLines += 1;
            } else if (lex_Current.token == T_POP_END) {
                return true;
            } else {
                prj_Error(ERROR_SYNTAX);
                r = false;
            }
        }
    }

    return r;
}
