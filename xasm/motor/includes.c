/*  Copyright 2008-2022 Carsten Elton Sorensen and contributors

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
#include "lexer_context.h"
#include "includes.h"


/* Internal variables */

static vec_t*
g_includePaths;


/* Private functions */
static string*
buildFilename(string* workingName, string* fileName) {
	if (workingName == NULL)
		return fcanonicalizePath(fileName);

	return freplaceFileComponent(workingName, fileName);
}

static string*
appendFilename(string* directory, string* fileName) {
	if (directory == NULL)
		return fcanonicalizePath(fileName);

	string* n = str_Concat(directory, fileName);
	string* r = fcanonicalizePath(n);
	str_Free(n);
	return r;
}



/* Public functions */

extern string*
inc_FindFile(string* fileName) {
	string* workingName = lex_Context == NULL ? NULL : lex_Context->buffer.name;

	string* candidate = buildFilename(workingName, fileName);
	if (candidate != NULL) {
		if (fexists(str_String(candidate))) {
			return candidate;
		}
		str_Free(candidate);
	}

    if (g_includePaths != NULL) {
        for (size_t count = 0; count < strvec_Count(g_includePaths); ++count) {
            string* candidate = appendFilename(strvec_StringAt(g_includePaths, count), fileName);

            if (fexists(str_String(candidate))) {
                return candidate;
            }
        }
    }

    if (workingName == NULL) {
        if (fexists(str_String(fileName))) {
            return fileName;
        }
    }

	str_Free(fileName);
    return NULL;
}


extern void
inc_AddIncludePath(string* pathname) {
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
