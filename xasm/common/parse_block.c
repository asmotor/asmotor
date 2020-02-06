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

#include <memory.h>

/* From util */
#include "mem.h"

/* From xasm */
#include "parse_block.h"
#include "lexer.h"
#include "filestack.h"


/* Internal functions */

#define REPT_LEN 4
#define ENDR_LEN 4
#define ELSE_LEN 4
#define ENDC_LEN 4
#define MACRO_LEN 5
#define ENDM_LEN 4

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


/* Public functions */

bool
parse_SkipTrueBranch(void) {
    size_t index = skipLine(0);
    while ((index = findControlToken(index)) != SIZE_MAX) {
        if (!skipRept(&index) && !skipIf(&index) && !skipMacro(&index)) {
            if (isEndc(index)) {
                fstk_Current->lineNumber += (uint32_t) lex_SkipBytes(index);
                return true;
            } else if (isElse(index)) {
                fstk_Current->lineNumber += (uint32_t) lex_SkipBytes(index + ELSE_LEN);
                return true;
            } else {
                index = skipLine(index);
            }
        }
    }

    return false;
}

bool
parse_SkipToEndc(void) {
    size_t index = skipLine(0);
    while ((index = findControlToken(index)) != SIZE_MAX) {
        if (!skipRept(&index) && !skipIf(&index) && !skipMacro(&index)) {
            if (isEndc(index)) {
                fstk_Current->lineNumber += (uint32_t) lex_SkipBytes(index);
                return true;
            } else {
                index = skipLine(index);
            }
        }
    }

    return 0;
}

bool
parse_CopyReptBlock(char** reptBlock, size_t* size) {
    size_t len = getReptBodySize(0);

    if (len == 0)
        return false;

    *size = len;

    *reptBlock = (char*) mem_Alloc(len + 1);
    lex_GetChars(*reptBlock, len);
    fstk_Current->lineNumber += (uint32_t) lex_SkipBytes(ENDR_LEN);

    return true;
}

bool
parse_CopyMacroBlock(char** dest, size_t* size) {
    size_t len = getMacroBodySize(0);

    *size = len;

    *dest = (char*) mem_Alloc(len + 1);
    fstk_Current->lineNumber += (uint32_t) lex_GetChars(*dest, len);
    fstk_Current->lineNumber += (uint32_t) lex_SkipBytes(ENDM_LEN);
    return true;
}

