#pragma once

#include "str.h"


typedef struct {
	const char* label;
	bool export;
	const char* operation;
	const char** arguments;
	char* line;
} SLexLine;

typedef struct {
	size_t totalLines;
	const SLexLine* lines;
} SLexBuffer;


extern SLexBuffer*
buf_CreateFromFile(const string* filename);

