/*  Copyright 2008-2023 Carsten Elton Sorensen and contributors

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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mem.h"
#include "str.h"
#include "strbuf.h"
#include "strcoll.h"
#include "vec.h"

#include "error.h"
#include "group.h"
#include "patch.h"
#include "section.h"
#include "xlink.h"

typedef struct {
	int count;
	MemoryPool** pools;
} Pools;

static Pools*
allocPools(int reserve) {
	Pools* pools = mem_Alloc(sizeof(Pools));
	pools->count = reserve;
	pools->pools = reserve == 0 ? NULL : mem_Alloc(sizeof(MemoryPool*) * reserve);

	return pools;
}

static void
dummyFree(intptr_t userData, intptr_t element) {}

#define DELIMITERS " \t\n$%%+-*/()[]:@;,."

static bool g_parsePool;
static bool g_parseSymbol;

static const char* token;
static size_t token_length;
static uint32_t pool_index;

static const char* g_filename;
static int g_line;

static SSection* g_linkerSymbols = NULL;

#define FERROR(fmt, ...) error("%s:%d: " fmt, g_filename, g_line, __VA_ARGS__)
#define ERROR(err)       error("%s:%d: %s", g_filename, g_line, err)

static void
appendLong(string_buffer* expr, uint8_t op, uint32_t value) {
	strbuf_AppendChar(expr, op);
	strbuf_AppendChar(expr, value & 0xFF);
	strbuf_AppendChar(expr, (value >> 8) & 0xFF);
	strbuf_AppendChar(expr, (value >> 16) & 0xFF);
	strbuf_AppendChar(expr, (value >> 24));
}

static void
appendConstant(string_buffer* expr, uint32_t constant) {
	appendLong(expr, OBJ_CONSTANT, constant);
}

static void
nextToken(const char** in) {
	while (**in == ' ' || **in == '\t')
		++*in;

	switch (**in) {
		case 0:
		case ';':
			token_length = 0;
			return;
		case '$':
		case '%':
		case '+':
		case '-':
		case '*':
		case '/':
		case '(':
		case ')':
		case '[':
		case ']':
		case ':':
		case '@':
		case ',':
		case '.':
			token = *in;
			token_length = 1;
			++*in;
			break;
		default:
			token = *in;
			token_length = 0;
			while (**in != 0 && strchr(DELIMITERS, **in) == NULL) {
				++*in;
				++token_length;
			}
			break;
	}
}

static bool
tokenIs(const char* s) {
	if (strlen(s) != token_length)
		return false;

	return strncmp(s, token, token_length) == 0;
}

static void
expectToken(const char** line, const char* s) {
	if (!tokenIs(s))
		FERROR("%s expected", s);

	nextToken(line);
}

static void
freePools(intptr_t userData, intptr_t element) {
	Pools* pools = (Pools*) element;
	for (int i = 0; i < pools->count; ++i) {
		pool_Free(pools->pools[i]);
	}
	mem_Free(pools->pools);
	mem_Free(pools);
}

static bool
parseInteger(const char** line, string_buffer* expr) {
	if (token_length == 0)
		return false;

	int base = 10;
	if (tokenIs("$")) {
		nextToken(line);
		base = 16;
	} else if (tokenIs("%%")) {
		nextToken(line);
		base = 2;
	}

	uint32_t value = 0;
	for (size_t i = 0; i < token_length; ++i) {
		char ch = token[i];
		if (ch >= '0' && ch <= '9')
			ch -= '0';
		else if (ch >= 'A' && ch <= 'Z')
			ch -= 'A' - 10;
		else if (ch >= 'a' && ch <= 'z')
			ch -= 'a' - 10;
		else
			return false;

		if (ch >= base)
			return false;

		value = value * base + ch;
	}

	appendConstant(expr, value);

	nextToken(line);

	return true;
}

static bool
parseExpression_1(const char** line, string_buffer* expr);

static bool
parseExpression_4(const char** line, string_buffer* expr) {
	if (g_parsePool && tokenIs("@")) {
		nextToken(line);

		appendConstant(expr, pool_index);

		return true;
	}

	if (parseInteger(line, expr)) {
		return true;
	}

	if (g_parseSymbol && token_length != 0) {
		string* name = str_CreateLength(token, token_length);
		MemoryGroup* group = group_Find(str_String(name));

		if (group == NULL) {
			FERROR("Group \"%s\" not found", str_String(name));
		}

		str_Free(name);

		appendConstant(expr, group->groupId);

		nextToken(line);
		expectToken(line, ".");

		string* property = str_CreateLength(token, token_length);

		if (strcmp(str_String(property), "start") == 0) {
			appendConstant(expr, PROP_START);
		} else if (strcmp(str_String(property), "size") == 0) {
			appendConstant(expr, PROP_SIZE);
		} else {
			FERROR("Property \"%s\" unknown", str_String(property));
		}

		strbuf_AppendChar(expr, OBJ_GROUP_PROPERTY);

		str_Free(property);

		nextToken(line);
	}

	return false;
}

static bool
parseExpression_3(const char** line, string_buffer* expr) {
	while (tokenIs("+"))
		nextToken(line);

	if (tokenIs("-")) {
		nextToken(line);
		appendConstant(expr, 0);
		if (parseExpression_3(line, expr)) {
			strbuf_AppendChar(expr, OBJ_OP_SUB);
			return true;
		}
	} else if (tokenIs("(")) {
		nextToken(line);
		if (parseExpression_1(line, expr)) {
			if (tokenIs(")")) {
				nextToken(line);
				return true;
			}
		}
	} else {
		return parseExpression_4(line, expr);
	}

	return false;
}

static bool
parseExpression_2(const char** line, string_buffer* expr) {
	if (parseExpression_3(line, expr)) {
		while (true) {
			bool mul = tokenIs("*");
			bool div = tokenIs("/");
			if (mul || div) {
				nextToken(line);
				if (parseExpression_3(line, expr)) {
					if (mul)
						strbuf_AppendChar(expr, OBJ_OP_MUL);
					else /* if (div) */
						strbuf_AppendChar(expr, OBJ_OP_DIV);
				}
			} else {
				break;
			}
		}
		return true;
	}

	return false;
}

static bool
parseExpression_1(const char** line, string_buffer* expr) {
	if (parseExpression_2(line, expr)) {
		while (true) {
			bool plus = tokenIs("+");
			bool minus = tokenIs("-");
			if (plus || minus) {
				nextToken(line);
				if (parseExpression_2(line, expr)) {
					if (plus)
						strbuf_AppendChar(expr, OBJ_OP_ADD);
					else /* if (minus) */
						strbuf_AppendChar(expr, OBJ_OP_SUB);
				}
			} else {
				break;
			}
		}
		return true;
	}

	return false;
}

static bool
parseExpression(const char** line, uint32_t* value) {
	SSymbol* symbol = NULL;
	string_buffer* buffer = strbuf_Create();
	bool r = false;

	*value = 0;

	if (parseExpression_1(line, buffer)) {
		int32_t ivalue;

		r = patch_EvaluateExpression((uint8_t*) strbuf_Data(buffer), strbuf_Size(buffer), &ivalue, &symbol, NULL);
		*value = (uint32_t) ivalue;
	}

	strbuf_Free(buffer);

	return r;
}

static uint32_t
parseOptionalExpression(const char** line) {
	uint32_t value;
	if (parseExpression(line, &value))
		return value;

	return UINT32_MAX;
}

static uint32_t
expectExpression(const char** line) {
	uint32_t r;
	if (!parseExpression(line, &r))
		ERROR("Error in expression");

	return r;
}

static MemoryPool*
parsePool(const char** line) {
	uint32_t cpu_address, cpu_bank, size;

	g_parsePool = true;
	if (parseExpression(line, &cpu_address) && parseExpression(line, &cpu_bank) && parseExpression(line, &size)) {
		uint32_t image_offset = parseOptionalExpression(line);
		uint32_t overlay = UINT32_MAX;
		if (tokenIs(":")) {
			nextToken(line);
			overlay = expectExpression(line);
		}
		g_parsePool = false;
		return pool_Create(image_offset, overlay, cpu_address, cpu_bank, size, false);
	}

	ERROR("Error in POOL definition");
	return NULL;
}

static void
parsePoolDirective(const char** line, strmap_t* pool_map) {
	if (token_length == 0)
		ERROR("Empty POOL definition");

	string* name = str_CreateLength(token, token_length);
	nextToken(line);

	Pools* pools = allocPools(1);
	pools->pools[0] = parsePool(line);
	strmap_Insert(pool_map, name, (intptr_t) pools);
}

static void
parseSymbolDirective(const char** line) {
	if (token_length == 0)
		ERROR("Empty SYMBOL definition");

	if (g_linkerSymbols == NULL) {
		g_linkerSymbols = sect_CreateNew();
		g_linkerSymbols->used = true;
	}

	g_linkerSymbols->symbols = mem_Realloc(g_linkerSymbols->symbols, sizeof(SSymbol) * (g_linkerSymbols->totalSymbols + 1));
	SSymbol* symbol = &g_linkerSymbols->symbols[g_linkerSymbols->totalSymbols++];

	strncpy(symbol->name, token, token_length);
	symbol->name[token_length] = 0;
	symbol->type = SYM_LINKER;
	symbol->value = 0;
	symbol->resolved = false;
	symbol->fileInfoIndex = UINT32_MAX;
	symbol->lineNumber = UINT32_MAX;
	symbol->section = g_linkerSymbols;
	symbol->expression = strbuf_Create();

	nextToken(line);

	g_parseSymbol = true;
	parseExpression_1(line, symbol->expression);
	g_parseSymbol = false;
}

static void
parsePoolsDirective(const char** line, strmap_t* pool_map) {
	if (token_length == 0)
		ERROR("Empty POOLS definition");

	string* name = str_CreateLength(token, token_length);
	nextToken(line);

	expectToken(line, "[");
	uint32_t range_start = expectExpression(line);
	expectToken(line, ":");
	uint32_t range_end = expectExpression(line);
	if (!tokenIs("]"))
		ERROR("Expected ]");

	Pools* pools = allocPools(range_end - range_start + 1);
	for (pool_index = range_start; pool_index <= range_end; ++pool_index) {
		const char* params = *line;
		nextToken(&params);
		pools->pools[pool_index - range_start] = parsePool(&params);
	}
	strmap_Insert(pool_map, name, (intptr_t) pools);
}

static void
parseGroupDirective(const char** line, strmap_t* pool_map) {
	if (token_length == 0)
		ERROR("Missing GROUP name");

	string* name = str_CreateLength(token, token_length);
	nextToken(line);

	if (tokenIs(":")) {
		// Skip token kind, the linker doesn't use it
		nextToken(line);
		nextToken(line);
	}

	if (token_length == 0)
		ERROR("Empty GROUP definition");

	vec_t* pool_list = vec_Create(dummyFree);
	int total = 0;
	while (token_length != 0) {
		string* name = str_CreateLength(token, token_length);

		Pools* pools;
		if (strmap_Value(pool_map, name, (intptr_t*) &pools)) {
			vec_PushBack(pool_list, (intptr_t) pools);
			total += pools->count;
		} else {
			FERROR("Unknown pool \"%s\"", str_String(name));
		}

		nextToken(line);
		str_Free(name);
	}

	MemoryGroup* group = group_Create(str_String(name), total);
	total = 0;
	for (size_t i = 0; i < vec_Count(pool_list); ++i) {
		Pools* pools = (Pools*) vec_ElementAt(pool_list, i);
		for (int j = 0; j < pools->count; ++j) {
			group->pools[total++] = pools->pools[j];
		}
	}

	vec_Free(pool_list);
	str_Free(name);
}

static void
parseFormat(const char** line) {
	if (tokenIs("CBM_PRG")) {
		nextToken(line);
		expectToken(line, "[");
		g_cbmHeaderAddress = expectExpression(line);
		expectToken(line, "]");
		g_allowedFormats |= FILE_FORMAT_CBM_PRG;
	} else if (tokenIs("BIN")) {
		g_allowedFormats |= FILE_FORMAT_BINARY;
	} else if (tokenIs("GAME_BOY")) {
		g_allowedFormats |= FILE_FORMAT_GAME_BOY;
	} else if (tokenIs("HUNK_EXE")) {
		g_allowedFormats |= FILE_FORMAT_AMIGA_EXECUTABLE;
	} else if (tokenIs("HUNK_OBJ")) {
		g_allowedFormats |= FILE_FORMAT_AMIGA_LINK_OBJECT;
	} else if (tokenIs("MEGA_DRIVE")) {
		g_allowedFormats |= FILE_FORMAT_MEGA_DRIVE;
	} else if (tokenIs("MASTER_SYSTEM")) {
		g_allowedFormats |= FILE_FORMAT_MASTER_SYSTEM;
	} else if (tokenIs("HC800_KERNEL")) {
		g_allowedFormats |= FILE_FORMAT_HC800_KERNEL;
	} else if (tokenIs("HC800")) {
		g_allowedFormats |= FILE_FORMAT_HC800;
	} else if (tokenIs("PGZ")) {
		g_allowedFormats |= FILE_FORMAT_PGZ;
	} else if (tokenIs("KUP")) {
		g_allowedFormats |= FILE_FORMAT_F256_KUP;
	} else if (tokenIs("KUPP")) {
		g_allowedFormats |= FILE_FORMAT_F256_KUP_PAD;
	} else if (tokenIs("COCO_QL")) {
		g_allowedFormats |= FILE_FORMAT_COCO_BIN;
	} else if (tokenIs("MEGA65_PRG")) {
		g_allowedFormats |= FILE_FORMAT_MEGA65_PRG;
	} else {
		FERROR("Unknown format \"%.*s\"", token_length, token);
	}

	nextToken(line);
}

static void
parseFormatsDirective(const char** line) {
	if (token_length == 0)
		ERROR("Empty FORMATS definition");

	while (token_length != 0) {
		parseFormat(line);
	}
}

static void
parseLine(const char* line, strmap_t* pools) {
	nextToken(&line);

	if (token_length == 0)
		return;

	if (tokenIs("POOL")) {
		nextToken(&line);
		g_parsePool = true;
		parsePoolDirective(&line, pools);
		g_parsePool = false;
	} else if (tokenIs("POOLS")) {
		nextToken(&line);
		parsePoolsDirective(&line, pools);
	} else if (tokenIs("GROUP")) {
		nextToken(&line);
		parseGroupDirective(&line, pools);
	} else if (tokenIs("SYMBOL")) {
		nextToken(&line);
		parseSymbolDirective(&line);
	} else if (tokenIs("FORMATS")) {
		nextToken(&line);
		parseFormatsDirective(&line);
	} else {
		FERROR("Unknown keyword \"%.*s\" in memory map", token_length, token);
	}

	if (token_length == 0) {
		return;
	}

	FERROR("Excess characters \"%.*s\" at end of line", token_length, token);
}

void
mdef_Read(const string* filename) {
	strmap_t* pools = strmap_Create(freePools);
	FILE* file = fopen(str_String(filename), "rt");

	if (file == NULL)
		FERROR("Unable to open file \"%s\"", filename);

	g_filename = str_String(filename);
	g_line = 1;
	g_parsePool = false;
	g_parseSymbol = false;
	string* line = NULL;
	while ((line = str_ReadLineFromFile(file)) != NULL) {
		parseLine((char*) str_String(line), pools);
		str_Free(line);
		g_line += 1;
	}

	if (g_allowedFormats == 0) {
		ERROR("Missing FORMATS");
	}
}
