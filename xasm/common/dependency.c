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

typedef struct Dependency {
    string* filename;
    struct Dependency* next;
} SDependency;


static string* g_outputFilename = NULL;
static string* g_mainOutput = NULL;
static SDependency* g_dependencies = NULL;
static SDependency** g_nextDependency = &g_dependencies;


void
dep_SetOutputFilename(const char* filename) {
    g_outputFilename = str_Create(filename);
}

extern void
dep_SetMainOutput(string* filename) {
    g_mainOutput = str_Copy(filename);
}

static bool
hasDependency(string* filename) {
    for (SDependency* dependency = g_dependencies; dependency != NULL; dependency = dependency->next) {
        if (str_Equal(filename, dependency->filename))
            return true;
    }

    return false;
}

extern void
dep_AddDependency(string* filename) {
    if (!hasDependency(filename)) {
        SDependency* dependency = (SDependency*) mem_Alloc(sizeof(SDependency));
        dependency->filename = str_Copy(filename);
        dependency->next = NULL;
        *g_nextDependency = dependency;
        g_nextDependency = &dependency->next;
    }
}

extern void
dep_WriteDependencyFile(void) {
    FILE* fileHandle = fopen(str_String(g_outputFilename), "wt");
    if (fileHandle != NULL) {
        fprintf(fileHandle, "%s:", str_String(g_mainOutput));
        for (SDependency* dependency = g_dependencies; dependency != NULL; dependency = dependency->next) {
            fprintf(fileHandle, " %s", str_String(dependency->filename));
        }
        fprintf(fileHandle, "\n\n");
        for (SDependency* dependency = g_dependencies->next; dependency != NULL; dependency = dependency->next) {
            fprintf(fileHandle, "%s:\n\n", str_String(dependency->filename));
        }
    }
}

