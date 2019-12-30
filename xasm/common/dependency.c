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

#include "asmotor.h"
#include "file.h"
#include "mem.h"
#include "str.h"
#include "strcoll.h"

/* Internal variables */

static string* g_outputFilename = NULL;
static string* g_mainOutput = NULL;
static string* g_mainDependency = NULL;
static set_t * g_dependencySet = NULL;


/* Internal functions */

static void
writeDependency(intptr_t element, intptr_t data) {
    FILE* fileHandle = (FILE*) data;
    string* str = (string*) element;
    fprintf(fileHandle, " %s", str_String(str));
}

static void
writeTarget(intptr_t element, intptr_t data) {
    FILE* fileHandle = (FILE*) data;
    string* str = (string*) element;
    fprintf(fileHandle, "%s:\n\n", str_String(str));
}


/* Exported functions */

extern void
dep_Initialize(const char* outputFileName) {
    g_outputFilename = str_Create(outputFileName);
    g_dependencySet = strset_Create();
}

extern void
dep_SetMainOutput(string* filename) {
    g_mainOutput = str_Copy(filename);
}

extern void
dep_AddDependency(string* filename) {
    if (g_dependencySet != NULL) {
        if (g_mainDependency == NULL) {
            g_mainDependency = str_Copy(filename);
        }
        strset_Insert(g_dependencySet, filename);
    }
}

extern void
dep_WriteDependencyFile(void) {
    if (g_dependencySet != NULL) {
        FILE* fileHandle = fopen(str_String(g_outputFilename), "wt");
        if (fileHandle != NULL) {
            fprintf(fileHandle, "%s:", str_String(g_mainOutput));
            set_ForEachElement(g_dependencySet, writeDependency, (intptr_t) fileHandle);

            fprintf(fileHandle, "\n\n");
            strset_Remove(g_dependencySet, g_mainDependency);
            set_ForEachElement(g_dependencySet, writeTarget, (intptr_t) fileHandle);
        }
    }
}
