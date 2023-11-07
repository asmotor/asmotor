#pragma once

#include "str.h"
#include "symbol.h"


typedef struct {
	const char* label;
	bool export;
	const char* operation;
	size_t totalArguments;
	const char** arguments;
	char* line;
} SLexLine;

typedef struct {
	const string* name;
	size_t totalLines;
	const SLexLine* lines;
} SLexerBuffer;


extern SLexerBuffer*
buf_CreateFromFile(const string* filename);

