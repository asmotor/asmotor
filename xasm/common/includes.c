/*  Copyright 2008-2017 Carsten Elton Sorensen

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

// From xasm
#include "xasm.h"
#include "includes.h"


/* Internal variables */

static vec_t*
g_includePaths;


/* Private functions */


/* Public functions */

extern string*
inc_FindFile(string* fileName) {
	string* workingName = fstk_Current->name;

    fileName = fcanonicalizePath(fileName);

    if (workingName == NULL) {
        if (fexists(str_String(fileName))) {
            return str_Copy(fileName);
        }
    }

	string* candidate = freplaceFileComponent(workingName, fileName);
	if (candidate != NULL) {
		if (fexists(str_String(candidate))) {
			return str_Copy(candidate);
		}
	}

	for (size_t count = 0; count < strvec_Count(g_includePaths); ++count) {
		string* candidate = str_Concat(strvec_StringAt(g_includePaths, count), fileName);

		if (fexists(str_String(candidate))) {
			return str_Copy(candidate);
		}
    }

    return NULL;
}


extern void
inc_AddIncludePath(string* pathname) {
    char ch;

    ch = str_CharAt(pathname, str_Length(pathname) - 1);
    if (ch != '\\' && ch != '/') {
        string* slash = str_Create("/");
        strvec_PushBack(g_includePaths, str_Concat(pathname, slash));
        str_Free(slash);
    } else {
        strvec_PushBack(g_includePaths, pathname);
    }
}
