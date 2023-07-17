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

#include "str.h"
#include "strbuf.h"
#include "strcoll.h"

#include "error.h"
#include "group.h"
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

/*
static void
addPool(Pools* pools, MemoryPool* pool) {
	pools->pools = realloc(pools->pools, sizeof(MemoryPool*) * (pools->count + 1));
	pools->pools[pools->count++] = pool;
}
*/


static void
dummyFree(intptr_t userData, intptr_t element) {
}


#define DELIMITERS " \t\n$%%+-*/()[]:@"

const char* token;
size_t token_length;
uint32_t pool_index;

static void
nextToken(const char** in) {
	while (**in == ' ' || **in == '\t')
		++*in;

	switch (**in) {
		case 0:
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
		error("%s expected", s);

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
parseInteger(const char** line, uint32_t* value) {
	if (token_length == 0)
		error("Invalid integer in linker map");

	int base = 10;
	if (tokenIs("$")) {
		nextToken(line);
		base = 16;
	} else if (tokenIs("%%")) {
		nextToken(line);
		base = 2;
	}

	*value = 0;
	for (size_t i = 0; i < token_length; ++i) {
		char ch = token[i];
		if (ch >= '0' && ch < '9')
			ch -= '0';
		else if (ch >= 'A' && ch <= 'Z')
			ch -= 'A' - 10;
		else if (ch >= 'a' && ch <= 'z')
			ch -= 'a' - 10;
		else
			return false;
		
		*value = *value * base + ch;
	}

	nextToken(line);

	return true;
}


static bool parseExpression_1(const char** line, uint32_t* value);

static bool
parseExpression_4(const char** line, uint32_t* value) {
	if (tokenIs("@")) {
		nextToken(line);
		*value = pool_index;
		return true;
	}

	if (parseInteger(line, value)) {
		return true;
	}

	return false;
}

static bool
parseExpression_3(const char** line, uint32_t* value) {
	while (tokenIs("+"))
		nextToken(line);

	if (tokenIs("-")) {
		nextToken(line);
		if (parseExpression_3(line, value)) {
			*value = - *value;
			return true;
		}
	} else if (tokenIs("(")) {
		nextToken(line);
		if (parseExpression_1(line, value)) {
			if (tokenIs(")")) {
				nextToken(line);
				return true;
			}
		}
	} else {
		return parseExpression_4(line, value);
	}

	return false;
}


static bool
parseExpression_2(const char** line, uint32_t* value) {
	if (parseExpression_3(line, value)) {
		while (true) {
			bool mul = tokenIs("*");
			bool div = tokenIs("/");
			if (mul || div) {
				uint32_t rhs;
				nextToken(line);
				if (parseExpression_3(line, &rhs)) {
					if (mul)
						*value *= rhs;
					else /* if (div) */
						*value /= rhs;
					
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
parseExpression_1(const char** line, uint32_t* value) {
	if (parseExpression_2(line, value)) {
		while (true) {
			bool plus = tokenIs("+");
			bool minus = tokenIs("-");
			if (plus || minus) {
				uint32_t rhs;
				nextToken(line);
				if (parseExpression_2(line, &rhs)) {
					if (plus)
						*value += rhs;
					else /* if (minus) */
						*value -= rhs;
					
				}
			} else {
				break;
			}
		}
		return true;
	}

	return false;
}


#define parseExpression parseExpression_1


/*
static bool
parseExpression(const char** line, uint32_t* value) {
	const char* t = *line;

	if (parseExpression_1(line, value))
		return true;

	error("Expression %s failed", t);		
}
*/


static uint32_t
parseOffsetExpression(const char** line) {
	uint32_t value;
	if (parseExpression(line, &value))
		return value;

	return -1;
}


static uint32_t
expectExpression(const char** line) {
	uint32_t r;
	if (!parseExpression(line, &r))
		error("Error in expression");

	return r;
}


static MemoryPool*
parsePool(const char** line) {
	uint32_t cpu_address, cpu_bank, size;
	if (parseExpression(line, &cpu_address) && parseExpression(line, &cpu_bank) && parseExpression(line, &size)) {
 		uint32_t image_offset = parseOffsetExpression(line);
		return pool_Create(image_offset, cpu_address, cpu_bank, size, false);
	}

	error("Error in pool definition");
	return NULL;
}


static void
parsePoolDirective(const char** line, strmap_t* pool_map) {
	string* name = str_CreateLength(token, token_length);
	nextToken(line);

	Pools* pools = allocPools(1);
	pools->pools[0] = parsePool(line);
	strmap_Insert(pool_map, name, (intptr_t) pools);
}


static void
parsePoolsDirective(const char** line, strmap_t* pool_map) {
	string* name = str_CreateLength(token, token_length);
	nextToken(line);

	expectToken(line, "[");
	uint32_t range_start = expectExpression(line);
	expectToken(line, ":");
	uint32_t range_end = expectExpression(line);
	if (!tokenIs("]"))
		error("Expected ]");

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
	string* name = str_CreateLength(token, token_length);
	nextToken(line);

	vec_t* pool_list = vec_Create(dummyFree);
	int total = 0;
	while (token_length != 0) {
		string* name = str_CreateLength(token, token_length);

		Pools* pools;
		if (strmap_Value(pool_map, name, (intptr_t*) &pools)) {
			vec_PushBack(pool_list, (intptr_t) pools);
			total += pools->count;
		} else {
			error("Unknown pool %s", str_String(name));
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
		g_cbmBaseAddress = expectExpression(line);
		expectToken(line, "]");
		g_allowedFormats |= FILE_FORMAT_CBM_PRG;

		return;
	}

	if (tokenIs("BIN")) {
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
	} else if (tokenIs("COCO_QL")) {
		g_allowedFormats |= FILE_FORMAT_COCO_BIN;
	} else {
		error("Unknown format %*s", token_length, token);
	}

	nextToken(line);
}


static void
parseFormatsDirective(const char** line) {
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
		parsePoolDirective(&line, pools);
	} else if (tokenIs("POOLS")) {
		nextToken(&line);
		parsePoolsDirective(&line, pools);
	} else if (tokenIs("GROUP")) {
		nextToken(&line);
		parseGroupDirective(&line, pools);
	} else if (tokenIs("FORMATS")) {
		nextToken(&line);
		parseFormatsDirective(&line);
	} else {
		error("Unknown keyword %s in memory map", token);
	}
}


static string*
readLine(FILE* file) {
	string_buffer* buf = strbuf_Create();
	
	int ch = fgetc(file);
	if (ch == EOF)
		return NULL;

	do {
		if (ch == '\n' || ch == EOF)
			break;
		strbuf_AppendChar(buf, ch);
		ch = fgetc(file);
	} while (ch != '\n' && ch != EOF);

	string* r = strbuf_String(buf);
	strbuf_Free(buf);

	return r;
}


void
mmap_Read(const string* filename) {
	strmap_t* pools = strmap_Create(freePools); 
	FILE* file = fopen(str_String(filename), "rt");

	if (file == NULL)
		error("Unable to open file %s", filename);

	string* line = NULL;
	while ((line = readLine(file)) != NULL) {
		parseLine((char *) str_String(line), pools);
		str_Free(line);
	}
}
