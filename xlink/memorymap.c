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


#define DELIMITERS " \t\n$%%+-*/()"

const char* token;
size_t token_length;


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
freePool(intptr_t userData, intptr_t element) {
	pool_Free((MemoryPool*) element);
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
parseExpression_3(const char** line, uint32_t* value) {
	if (tokenIs("(")) {
		nextToken(line);
		if (parseExpression_1(line, value)) {
			if (tokenIs(")")) {
				nextToken(line);
				return true;
			}
		}
		return false;
	}

	if (tokenIs("+"))
		nextToken(line);

	bool negate = tokenIs("-");
	if (negate)
		nextToken(line);

	if (parseInteger(line, value)) {
		if (negate)
			*value = -*value;

		return true;
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


static void
parsePool(const char** line, strmap_t* pools) {
	string* name = str_CreateLength(token, token_length);
	nextToken(line);

	uint32_t image_offset, cpu_address, cpu_bank, size;
	if (parseExpression(line, &image_offset) && parseExpression(line, &cpu_address) && parseExpression(line, &cpu_bank) && parseExpression(line, &size)) {
		MemoryPool* pool = pool_Create(image_offset, cpu_address, cpu_bank, size);
		strmap_Insert(pools, name, (intptr_t) pool);
	} else {
		error("Error in pool definition");
	}
}


static void
parsePools(const char** line, strmap_t* pools) {
}


static void
parseGroup(const char** line, strmap_t* pools) {
}


static void
parseLine(const char* line, strmap_t* pools) {
	nextToken(&line);

	if (token_length == 0)
		return;

	if (tokenIs("POOL")) {
		nextToken(&line);
		parsePool(&line, pools);
	} else if (tokenIs("POOLS")) {
		nextToken(&line);
		parsePools(&line, pools);
	} else if (tokenIs("GROUP")) {
		nextToken(&line);
		parseGroup(&line, pools);
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
	strmap_t* pools = strmap_Create(freePool); 
	FILE* file = fopen(str_String(filename), "rt");

	if (file == NULL)
		error("Unable to open file %s", filename);

	string* line = NULL;
	while ((line = readLine(file)) != NULL) {
		str_Free(line);
		parseLine((char *) str_String(line), pools);
	}
}
