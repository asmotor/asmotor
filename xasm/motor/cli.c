/*  Copyright 2008-2026 Carsten Elton Sorensen and contributors

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"
#include "str.h"
#include "strcoll.h"
#include "util.h"

#include "cli.h"

#include "dependency.h"
#include "errors.h"
#include "lexer.h"
#include "options.h"
#include "parse.h"
#include "parse_expression.h"
#include "symbol.h"
#include "xasm.h"
extern void
cli_PrintUsage(void) {
	printf("%s v" ASMOTOR_VERSION "\n\nUsage: %s [options] asmfile\n"
	       "Options:\n"
	       "    -a <n>    Section alignment when writing binary file (default is %d bytes)\n"
	       "    -b <AS>   Change the two characters used for binary constants\n"
	       "              (default is 01)\n"
	       "    -d <FILE> Output dependency file for GNU Make\n"
	       "    -D <NAME> Define EQU symbol with the value 1, or NAME=value\n"
	       "              (value is any constant expression)\n"
	       "    -e (l|b)  Change endianness\n"
	       "    -f <F>    Output format, one of\n"
	       "                  x - xobj (default)\n"
	       "                  e - ELF object file\n"
	       "                  b - binary file\n"
	       "                  v - verilog readmemh file\n",
	       xasm_Configuration->executableName, xasm_Configuration->executableName, xasm_Configuration->sectionAlignment);

	if (xasm_Configuration->supportAmiga) {
		printf("                  g - Amiga executable file\n"
		       "                  h - Amiga object file\n");
	}

	printf("    -g        Include debug information\n"
	       "    -h        This text\n"
	       "    -i <dir>  Extra include path (can appear more than once)\n"
	       "    -o <f>    Write assembly output to <file>\n"
	       "    -s <file> Use section types from machine definition file\n"
	       "    -v        Verbose text output\n"
	       "    -w <d>    Disable warning <d> (four digits)\n"
	       "    -z <XX>   Set the byte value (hex format) used for uninitialised\n"
	       "              data (default is FF)\n"
	       "\n"
	       "Machine specific options:\n");
	xasm_Configuration->printOptionUsage();
	exit(EXIT_SUCCESS);
}
extern void
cli_Init(SCLiState* state) {
	state->format = 'x';
	state->verbose = false;
	state->outputFilename = NULL;
	state->cliDefines = strvec_Create();
}

extern void
cli_Free(SCLiState* state) {
	strvec_Free(state->cliDefines);
	str_Free(state->outputFilename);
}

/* An option that requires an argument but got none: warn and ignore. */
static bool
optionArgument(const char* option, const char* optArg) {
	if (optArg[0] != 0)
		return true;

	err_Warn(WARN_OPTION, option);
	return false;
}
static bool
formatSupported(char format) {
	if (strchr("xbv", format) != NULL)
		return true;

	if (format == 'e')
		return xasm_Configuration->supportELF;

	if (format == 'g' || format == 'h')
		return xasm_Configuration->supportAmiga;

	return false;
}

static void
handleOptionFormat(SCLiState* state, const char* option, const char* optArg) {
	if (optionArgument(option, optArg)) {
		if (formatSupported(optArg[0]))
			state->format = optArg[0];
		else
			err_Warn(WARN_OPTION, option);
	}
}
static void
handleOptionDependency(SCLiState* state, const char* option, const char* optArg) {
	if (optionArgument(option, optArg))
		dep_Initialize(optArg);
}

static void
handleOptionDefine(SCLiState* state, const char* option, const char* optArg) {
	if (optionArgument(option, optArg)) {
		string* def = str_Create(optArg);
		strvec_PushBack(state->cliDefines, def);
		str_Free(def);
	}
}

static void
handleOptionOutput(SCLiState* state, const char* option, const char* optArg) {
	if (optionArgument(option, optArg))
		state->outputFilename = str_Create(optArg);
}

static void
handleOptionSectionTypes(SCLiState* state, const char* option, const char* optArg) {
	if (optionArgument(option, optArg) && sym_ReadMachineDefinitionFile(optArg))
		opt_Current->createGroups = false;
}
/* Command-line help - takes no argument. */
static void
handleOptionHelp(SCLiState* state, const char* option, const char* optArg) {
	cli_PrintUsage();
}

/* Verbose text output - takes no argument. */
static void
handleOptionVerbose(SCLiState* state, const char* option, const char* optArg) {
	state->verbose = true;
}

/* Motor-wide options: argument validation lives in options.c and the
 * machine-specific extension in the CPU plugin, so the only decision here
 * is which spelling of the argument to blame in a warning. */
static void
handleOptionMotor(SCLiState* state, const char* option, const char* optArg) {
	const char* display = optArg == option + 2 ? &option[1] : option;
	opt_ParseLetter(option[1], optArg, display);
}
typedef struct OptionEntry {
	char letter;
	bool takesArgument;
	void (*handle)(SCLiState* state, const char* option, const char* optArg);
} SOptionEntry;

/* One entry per option letter: the table is the single source of truth for
 * which letters exist, which of them require an argument (and may therefore
 * swallow the next token), and which handler applies. */
static const SOptionEntry optionTable[] = {
	{ '?', false, handleOptionHelp },
	{ 'h', false, handleOptionHelp },
	{ 'd', true, handleOptionDependency },
	{ 'D', true, handleOptionDefine },
	{ 'f', true, handleOptionFormat },
	{ 'o', true, handleOptionOutput },
	{ 's', true, handleOptionSectionTypes },
	{ 'v', false, handleOptionVerbose },
	{ 'a', true, handleOptionMotor },
	{ 'b', true, handleOptionMotor },
	{ 'e', true, handleOptionMotor },
	{ 'g', false, handleOptionMotor },
	{ 'i', true, handleOptionMotor },
	{ 'm', true, handleOptionMotor },
	{ 'w', true, handleOptionMotor },
	{ 'z', true, handleOptionMotor },
};

static const SOptionEntry*
optionLookup(char letter) {
	for (size_t i = 0; i < sizeof(optionTable) / sizeof(optionTable[0]); ++i) {
		if (optionTable[i].letter == letter)
			return optionTable + i;
	}
	return NULL;
}

/* Scan the leading option tokens. Advances *argn and reduces *argc by the
 * number of tokens consumed, so the caller is left with the file arguments. */
extern void
cli_ParseOptions(SCLiState* state, int* argc, int* argn, char** argv) {
	while (*argc && argv[*argn][0] == '-') {
		const char* option = argv[*argn];
		const char* optArg = option[1] != 0 ? &option[2] : "";
		const SOptionEntry* entry = optionLookup(option[1]);
		bool spaced = entry != NULL && entry->takesArgument && optArg[0] == 0 && *argc > 1;

		if (spaced)
			optArg = argv[*argn + 1];

		if (entry != NULL)
			entry->handle(state, option, optArg);
		else
			err_Warn(WARN_OPTION, option);

		*argn += spaced ? 2 : 1;
		*argc -= spaced ? 2 : 1;
	}
}
/* Evaluate a constant expression that was given as a standalone string. The
 * trailing comma keeps the expression parser lookahead inside this buffer
 * instead of eating the first token of the real source file. */
static int32_t
evaluateConstantExpression(const char* value) {
	string* valueStr = str_CreateFormat("%s,", value);
	string* cmdName = str_Create("commandline");
	SLexerContext* ctx = lexctx_CreateMemoryContext(cmdName, valueStr, strvec_Create());
	ctx->type = CONTEXT_FILE;
	ctx->fileInfo = NULL;
	ctx->lineNumber = 1;
	list_Insert(lex_Context, ctx);

	parse_GetToken();
	int32_t result = parse_ConstantExpression();

	/* The lexer pops the buffer itself if the parser already read past the end. */
	if (lex_Context == ctx)
		lexctx_EndCurrentBuffer();

	str_Free(cmdName);
	str_Free(valueStr);
	return result;
}

/* Symbols defined on the command line carry no source location: that is what
 * distinguishes them from source symbols when a redefinition is reported. */
static void
defineCommandLineSymbol(string* name, int32_t value) {
	SSymbol* symbol = sym_CreateEqu(name, value);
	if (symbol != NULL) {
		symbol->fileInfo = NULL;
		symbol->lineNumber = 0;
	}
}

extern void
cli_EvaluateDefines(vec_t* cliDefines) {
	for (size_t i = 0; i < strvec_Count(cliDefines); ++i) {
		string* def = strvec_StringAt(cliDefines, i);
		const char* text = str_String(def);
		const char* assign = strchr(text, '=');
		size_t nameLength = assign != NULL ? (size_t) (assign - text) : strlen(text);

		if (nameLength == 0 || (assign != NULL && assign[1] == 0)) {
			err_Warn(WARN_OPTION, text);
		} else {
			string* name = str_CreateLength(text, nameLength);
			if (assign == NULL)
				defineCommandLineSymbol(name, 1);
			else
				defineCommandLineSymbol(name, evaluateConstantExpression(&assign[1]));
			str_Free(name);
		}

		str_Free(def);
	}
}
