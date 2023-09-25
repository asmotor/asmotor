#include <ctype.h>

#include "errors.h"
#include "lexbuffer.h"

#define MAX_LINE_LENGTH 1024


static char
peekChar(char** source) {
	char ch = 0;

	if (source && *source) {
		ch = **source != ';' ? **source : 0;
		if (!ch) {
			**source = 0;
			*source = NULL;
		}
	}

	return ch;
}


static bool
nextMatch(char** source, char ch) {
	if (peekChar(source) == ch) {
		*source += 1;
		return true;
	}

	return false;
}


static char*
terminateString(char* end) {
	if (end)
		*end++ = 0;

	return end;
}

static char*
skipWhile(char* end, bool (*predicate)(char)) {
	while (end != NULL && predicate(peekChar(&end))) {
		++end;
	}
	return end;
}


static bool
isWhitespace(char ch) {
	return isspace(ch) != 0;
}


static SLexBuffer*
allocLexBuffer(size_t reserveLines) {
	SLexBuffer* buffer = (SLexBuffer*) malloc(sizeof(SLexBuffer));
	buffer->totalLines = 0;
	buffer->lines = (SLexLine*) malloc(reserveLines * sizeof(SLexLine));

	return buffer;
}


static void
initializeLexLine(SLexLine* line) {
	line->label = NULL;
	line->export = false;
	line->operation = NULL;
	line->totalArguments = 0;
	line->arguments = NULL;
	line->line = NULL;
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
skipWhitespace(char* source) {
	return skipWhile(source, isWhitespace);
}


// --------------------------------------------------------------------------
// -- Arguments
// --------------------------------------------------------------------------

/*
static char*
parseArgument(SLexLine* lexLine, char* source) {
	char* end = source;
	while (*end && *end != ',') {
		switch (*end) {
			case '(':
				end = skipPast(end + 1, ')');
				break;
			case '[':
				end = skipPast(end + 1, ']');
				break;
			case '{':
				end = skipPast(end + 1, '}');
				break;
			default:
				++end;
		}

	}

	*end++ = 0;

	size_t i = lexLine->totalArguments++;
	lexLine->arguments = realloc(lexLine->arguments, sizeof(const char*) * lexLine->totalArguments);
	lexLine->arguments[i] = source;

	return skipWhitespace(end);
}


static char*
parseArguments(SLexLine* lexLine, char* source) {
	if (!source)
		return NULL;

	do {
		source = parseArgument(lexLine, source);
		if (*source != ',')
			break;

		source = skipWhitespace(source + 1);
	} while (source);

	return skipWhitespace(source);	
}
*/


// --------------------------------------------------------------------------
// -- Operations
// --------------------------------------------------------------------------

static bool
isOperationStartChar(char ch) {
	return ch == '_' || isalpha(ch);
}


static bool
isOperationChar(char ch) {
	return ch == '_' || isalnum(ch);
}


static char*
parseOperation(SLexLine* lexLine, char* source) {
	if (!source)
		return NULL;

	char ch = peekChar(&source);
	if (isOperationStartChar(ch)) {
		char* end = skipWhile(source + 1, isOperationChar);
		if (end == NULL || *end == 0 || isWhitespace(*end)) {
			end = terminateString(end);
			lexLine->operation = source;
			return skipWhitespace(end);
		}	
	}

	err_Error(ERROR_INVALID_OPERATION);
	return NULL;
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

	char* end = source;
	if (isspace(*end)) {
		return skipWhitespace(end);
	}

	char ch = peekChar(&end);
	if (isLabelStartChar(ch)) {
		end = skipWhile(end + 1, isLabelChar);

		if (nextMatch(&end, ':')) {
			if (nextMatch(&end, ':')) {
				lexLine->export = true;
				end = terminateString(end);
			}
			lexLine->label = source;
			return skipWhitespace(end);
		}

		ch = peekChar(&end);
		if (end == NULL || ch == 0 || isWhitespace(ch)) {
			end = terminateString(end);
			lexLine->label = source;
			return skipWhitespace(end);
		}
	} else if (isdigit(ch)) {
		++end;
		if (nextMatch(&end, '$')) {
			ch = peekChar(&end);
			if (ch == 0 || isspace(ch)) {
				end = terminateString(end);
				lexLine->label = source;
				return skipWhitespace(end);
			}
		}
	}

	err_Error(ERROR_INVALID_LABEL);
	return NULL;
}


static bool
parseLine(SLexLine* lexLine, const string* filename, FILE* file) {
	initializeLexLine(lexLine);

	char* line = malloc(MAX_LINE_LENGTH);
	if (readString(line, filename, file)) {
		char* p = line;
		p = parseLabel(lexLine, p);
		p = parseOperation(lexLine, p);
		//p = parseArguments(lexLine, p);

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

