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

#ifndef XASM_MOTOR_CLI_H_INCLUDED_
#define XASM_MOTOR_CLI_H_INCLUDED_

#include <stdbool.h>

#include "str.h"
#include "strcoll.h"
#include "util.h"

/* Accumulated result of the command-line option scan. */
typedef struct CLiState {
	char format;
	bool verbose;
	string* outputFilename;
	vec_t* cliDefines;
} SCLiState;

extern void
cli_PrintUsage(void);

extern void
cli_Init(SCLiState* state);

extern void
cli_Free(SCLiState* state);

/* Scan the leading option tokens. Advances *argn and reduces *argc by the
 * number of tokens consumed, so the caller is left with the file arguments.
 * Unknown options are reported here, like the other option errors. */
extern void
cli_ParseOptions(SCLiState* state, int* argc, int* argn, char** argv);

/* Define the symbols collected by -D. Must run once the lexer and the token
 * vocabulary are up, so constant expressions may use the source language. */
extern void
cli_EvaluateDefines(vec_t* cliDefines);

#endif /* XASM_MOTOR_CLI_H_INCLUDED_ */
