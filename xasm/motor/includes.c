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

// From util
#include "file.h"
#include "str.h"
#include "strcoll.h"
#include "types.h"
#include "vec.h"

// From xasm
#include "includes.h"
#include "lexer_context.h"

/* Internal variables */

static vec_t* g_includePaths;

/* Private functions */
static void
buildFilename(string** dest, const string* workingName, const string* fileName) {
	if (workingName == NULL) {
		string* r = fcanonicalizePath(fileName);
		str_Move(dest, &r);
		return;
	}

	freplaceFileComponent(dest, workingName, fileName);
}

static void
appendFilename(string** dest, const string* directory, const string* fileName) {
	if (directory == NULL) {
		string* r = fcanonicalizePath(fileName);
		str_Move(dest, &r);
		return;
	}

	string* n = str_Concat(directory, fileName);
	string* r = fcanonicalizePath(n);
	str_Free(n);
	str_Move(dest, &r);
}

/* Public functions */

/*
 * inc_FindFile: takes a BORROWED reference to fileName.
 * Writes result to *dest (owned by caller, or NULL if not found).
 * The caller always retains ownership of fileName and must free it.
 */
extern void
inc_FindFile(string** dest, const string* fileName) {
	string* workingName = lex_Context == NULL ? NULL : lex_Context->buffer.name;

	string* candidate = NULL;
	buildFilename(&candidate, workingName, fileName);
	if (candidate != NULL) {
		if (fexists(str_String(candidate))) {
			str_Move(dest, &candidate);
			return;
		}
		str_Free(candidate);
	}

	if (g_includePaths != NULL) {
		for (size_t count = 0; count < strvec_Count(g_includePaths); ++count) {
			appendFilename(&candidate, strvec_StringAt(g_includePaths, count), fileName);

			if (fexists(str_String(candidate))) {
				str_Move(dest, &candidate);
				return;
			}
			str_Free(candidate);
		}
	}

	if (workingName == NULL) {
		if (fexists(str_String(fileName))) {
			string* r = fcanonicalizePath(fileName);
			str_Move(dest, &r);
			return;
		}
	}

	str_Clear(dest);
}

extern void
inc_AddIncludePath(const string* pathname) {
	if (g_includePaths == NULL)
		g_includePaths = strvec_Create();

	char ch = str_CharAt(pathname, str_Length(pathname) - 1);
	if (ch != '\\' && ch != '/') {
		string* slash = str_Create("/");
		string* cat = str_Concat(pathname, slash);
		strvec_PushBack(g_includePaths, cat);
		str_Free(slash);
		str_Free(cat);
	} else {
		strvec_PushBack(g_includePaths, pathname);
	}
}
