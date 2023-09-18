#include <ctype.h>

#include "errors.h"
#include "lexbuffer.h"

#define MAX_LINE_LENGTH 1024


inline char
peekChar(const char* source) {
	if (!source || *source == ';') {
		return *source;
	}

	return 0;
}


inline char
nextChar(const char** source) {
	char ch = peekChar(*source);
	if (ch)
		++*source;

	return ch;
}


static SLexBuffer*
allocLexBuffer(size_t reserveLines) {
	SLexBuffer* buffer = (SLexBuffer*) malloc(sizeof(SLexBuffer));
	buffer->totalLines = 0;
	buffer->lines = (SLexLine*) malloc(reserveLines * sizeof(SLexLine));

	return buffer;
}


static SLexLine*
allocLexLine(void) {
	SLexLine* line = (SLexLine*) malloc(sizeof(SLexLine));
	line->label = NULL;
	line->export = false;
	line->operation = NULL;
	line->arguments = NULL;
	line->line = NULL;

	return line;
}


static bool
readString(char* line, const string* filename, FILE* file) {
	uint32_t lineNumber = 1;
	if (fgets(line, MAX_LINE_LENGTH, file) != NULL) {
		size_t len = strlen(line);

		if (len == MAX_LINE_LENGTH - 1) {
			err_Error(ERROR_LINE_TOO_LONG, str_String(filename), lineNumber);
		}

		// Trim return characters from end of line
		while ((len > 0 && line[len - 1] == '\r') || (line[len - 1] == '\n')) {
			len -= 1;
			line[len] = 0;
		}

		return true;
	}

	return false;
}


static char*
skipWhitespace(const char* source) {
	while (*source && isspace(*source))
		++source;

	return (char*) source;
}


static char*
skipWhitespaceAndComment(const char* source) {
	source = (const char*) skipWhitespace(source);
	return *source && *source != ';' ? (char*) source : NULL;
}


// --------------------------------------------------------------------------
// -- Labels
// --------------------------------------------------------------------------

static bool
isLabelStartChar(char ch) {
	return ch == '.' || ch == '_' || isalpha(ch);
}


static bool
isLabelChar(char ch) {
	return ch == '_' || isalnum(ch);
}


static char*
parseLabel(SLexLine* lexLine, char* source) {
	if (!source)
		return NULL;

	if (isspace(*source)) {
		return skipWhitespaceAndComment(source);
	}

	if (*source && isLabelStartChar(*source)) {
		char* end = source + 1;
		while (isLabelChar(*end))
			++end;
		
		if (end[0] == ':') {
			*end++ = 0;
			if (*end == ':') {
				lexLine->export = true;
				++end;
			}
			lexLine->label = source;
			return skipWhitespace(end);
		}

		if (isspace(*end)) {
			*end++ = 0;
			lexLine->label = source;
			return skipWhitespace(end);
		}
	} else if (*source && isdigit(*source)) {
		char* end = source + 1;
		if (*end == '$') {
			++end;
			if (isspace(*end)) {
				*end++ = 0;
				return skipWhitespace(end);
			}
		}
	}

	err_Error(ERROR_INVALID_LABEL);
	return NULL;
}


static bool
parseLine(SLexLine* lexLine, const string* filename, FILE* file) {
	char* line = malloc(MAX_LINE_LENGTH);
	if (readString(line, filename, file)) {
		char* p = line;
		SLexLine* lexLine = allocLexLine();

		p = parseLabel(lexLine, p);
		// p = parseOperation(lexLine, p);

		return true;
	}

	free(line);
	return false;
}


extern SLexBuffer*
buf_CreateFromFile(const string* filename) {
	size_t allocatedLines = 4;
	SLexBuffer* buffer = allocLexBuffer(allocatedLines);

	FILE* file = fopen(str_String(filename), "rb");
	do {
		if (allocatedLines <= buffer->totalLines) {
			allocatedLines += allocatedLines >> 1;
			buffer->lines = (SLexLine*) realloc((SLexLine*) buffer->lines, sizeof(SLexLine) * allocatedLines);
		}
		buffer->totalLines += 1;
	} while (parseLine((SLexLine*) &buffer->lines[buffer->totalLines - 1], filename, file));

	buffer->totalLines -= 1;
	buffer->lines = realloc((SLexLine*) buffer->lines, sizeof(SLexLine) * buffer->totalLines);

	return buffer;
}

