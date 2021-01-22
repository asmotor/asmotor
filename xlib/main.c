/*  Copyright 2008-2021 Carsten Elton Sorensen and contributors

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "util.h"
#include "file.h"
#include "types.h"

#include "library.h"

NORETURN(void fatalError(const char* s));
NORETURN(static void printUsage(void));

void
fatalError(const char* s) {
    printf("*ERROR* : %s\n", s);
    exit(EXIT_FAILURE);
}


static void
printUsage(void) {
    printf("xlib (ASMotor v" ASMOTOR_VERSION ")\n\n"
           "Usage: xlib library command [module1 [module2 [... modulen]]]\n"
           "Commands:\n\ta\tAdd/replace modules to library\n"
           "\td\tDelete modules from library\n"
           "\tl\tList library contents\n"
           "\tx\tExtract modules from library\n");
    exit(0);
}

static void
handleAddCommand(int argc, char* argv[], SModule* library, const char* libname) {
    int argn = 0;
    while (argc) {
        library = lib_AddReplace(library, argv[argn++]);
        argc -= 1;
    }
    lib_Write(library, libname);
}

static void
handleDeleteCommand(int argc, char* argv[], SModule* library, const char* libname) {
    int argn = 0;
    while (argc) {
        library = lib_DeleteModule(library, argv[argn++]);
        argc -= 1;
    }
    lib_Write(library, libname);
}

static void
handleListCommand(SModule* library) {
    for (SModule* current = library; current != NULL; current = current->nextModule) {
        printf("%10ld %s\n", (long) current->byteLength, current->name);
    }
}

static void
handleExtractCommand(int argc, char* argv[], SModule* library) {
    int argn = 0;
    while (argc) {
        SModule* module = lib_Find(library, argv[argn]);
        if (module != NULL) {
            FILE* fileHandle = fopen(argv[argn], "wb");

            if (fileHandle != NULL) {
                fwrite(module->data, sizeof(uint8_t), module->byteLength, fileHandle);
                fclose(fileHandle);
                printf("Extracted module \"%s\"\n", argv[argn]);
            } else
                fatalError("Unable to write module");
        } else
            fatalError("Module not found");

        argn += 1;
        argc -= 1;
    }
}

static bool
handleCommand(int argc, char* argv[], SModule* library, const char* libname) {
    int argn = 0;
    if (strlen(argv[argn]) == 1) {
        uint8_t command = argv[argn++][0];
        argc -= 1;
        switch (tolower(command)) {
            case 'a':
                handleAddCommand(argc, argv + argn, library, libname);
                return true;
            case 'd':
                handleDeleteCommand(argc, argv + argn, library, libname);
                return true;
            case 'l':
                handleListCommand(library);
                return true;
            case 'x':
                handleExtractCommand(argc, argv + argn, library);
                return true;
        }
    }

    return false;
}

extern int
main(int argc, char* argv[]) {
    argc -= 1;
    if (argc >= 2) {
        int32_t argn = 1;
        char* libname = argv[argn++];
        argc -= 1;

        SModule* library = lib_Read(libname);

        if (!handleCommand(argc, argv + argn, library, libname))
            fatalError("Invalid command");

        lib_Free(library);
    } else {
        printUsage();
    }

    return 0;
}
