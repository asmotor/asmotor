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

#include <ctype.h>
#include <stdarg.h>

#include "util.h"
#include "file.h"
#include "str.h"

#include "amiga.h"
#include "assign.h"
#include "commodore.h"
#include "foenix.h"
#include "gameboy.h"
#include "group.h"
#include "hc800.h"
#include "image.h"
#include "mapfile.h"
#include "object.h"
#include "patch.h"
#include "section.h"
#include "sega.h"
#include "smart.h"
#include "xlink.h"

typedef enum {
    FILE_FORMAT_BINARY,
    FILE_FORMAT_GAMEBOY,
    FILE_FORMAT_AMIGA_EXECUTABLE,
    FILE_FORMAT_AMIGA_LINK_OBJECT,
    FILE_FORMAT_CBM_PRG,
    FILE_FORMAT_MEGA_DRIVE,
    FILE_FORMAT_MASTER_SYSTEM,
    FILE_FORMAT_HC800_KERNAL,
    FILE_FORMAT_HC800,
    FILE_FORMAT_TGZ,
} FileFormat;

bool
format_SupportsReloc(FileFormat type) {
    return type == FILE_FORMAT_AMIGA_EXECUTABLE || type == FILE_FORMAT_AMIGA_LINK_OBJECT;
}

bool
format_SupportsOnlySectionRelativeReloc(FileFormat type) {
    return type == FILE_FORMAT_AMIGA_EXECUTABLE;
}

bool
format_SupportsImports(FileFormat type) {
    return type == FILE_FORMAT_AMIGA_LINK_OBJECT;
}

static void
printUsage(void) {
    printf("xlink (ASMotor v" ASMOTOR_VERSION ")\n"
           "\n"
           "Usage: xlink [options] file1 [file2 [... filen]]\n"
           "Options: (a forward slash (/) can be used instead of the dash (-))\n"
           "\t-h\t\tThis text\n"
           "\t-c\t\tMemory configuration\n"
           "\t    -cami\t\tAmiga\n"
           "\t    -ccbm64\tCommodore 64\n"
           "\t    -ccbm128\tCommodore 128 unbanked\n"
           "\t    -ccbm128f\tCommodore 128 Function ROM (32 KiB)\n"
           "\t    -ccbm128fl\tCommodore 128 Function ROM Low (16 KiB)\n"
           "\t    -ccbm128fh\tCommodore 128 Function ROM High (16 KiB)\n"
           "\t    -ccbm264\tCommodore 264/TED series\n"
           "\t    -cgb\t\tGame Boy ROM image\n"
           "\t    -cgbs\t\tGame Boy small mode (32 KiB)\n"
           "\t    -csmd\tSega Mega Drive\n"
           "\t    -csms8\tSega Master System (8 KiB)\n"
           "\t    -csms16\tSega Master System (16 KiB)\n"
           "\t    -csms32\tSega Master System (32 KiB)\n"
           "\t    -csms48\tSega Master System (48 KiB)\n"
           "\t    -csmsb\tSega Master System banked (64+ KiB)\n"
           "\t    -chc800b\tHC800 16 KiB ROM\n"
           "\t    -chc800s\tHC800 small mode (64 KiB text + data + bss)\n"
           "\t    -chc800sh\tHC800 small Harvard mode (64 KiB text, 64 KiB data + bss)\n"
           "\t    -chc800m\tHC800 medium mode (32 KiB text + data + bss, 32 KiB sized\n"
		   "\t          \tbanks text)\n"
           "\t    -chc800mh\tHC800 medium Harvard executable (32 KiB text + 32 KiB\n"
		   "\t           \tsized text banks, 64 KiB data + bss)\n"
           "\t    -chc800l\tHC800 large mode (32 KiB text + data + bss, 32 KiB sized\n"
		   "\t          \tbanks text + data + bss)\n"
           "\t    -cfxa2560x\tFoenix A2560X/K\n"
           "\t-f\t\tFile format\n"
           "\t    -famiexe\t\tAmiga executable\n"
           "\t    -familnk\t\tAmiga link object\n"
           "\t    -tbin\tBinary\n"
           "\t    -tcbm\tCommodore .PRG\n"
           "\t    -tgb\t\tGame Boy ROM image\n"
           "\t    -tsmd\tSega Mega Drive\n"
           "\t    -tsms\tSega Master System\n"
           "\t    -thc800\tHC800 executable\n"
           "\t    -tfxtgz\tFoenix TGZ\n"
           "\t-m<mapfile>\tWrite a mapfile to <mapfile>\n"
           "\t-o<output>\tWrite output to file <output>\n"
           "\t-s<symbol>\tPerform smart linking starting with <symbol>\n"
    );
    exit(EXIT_SUCCESS);
}

int
main(int argc, char* argv[]) {
    int argn = 1;
    bool targetDefined = false;
    FileFormat outputFormat = FILE_FORMAT_BINARY;
	uint8_t* hc800Config = NULL;
	uint16_t cbmBaseAddress = 0;
    int binaryPad = -1;

    const char* outputFilename = NULL;
    const char* smartlink = NULL;
    const char* mapFilename = NULL;

    if (argc == 1)
        printUsage();

    while (argn < argc && (argv[argn][0] == '-' || argv[argn][0] == '/')) {
        switch (tolower((unsigned char) argv[argn][1])) {
            case '?':
            case 'h':
                ++argn;
                printUsage();
                break;
            case 'm':
                /* MapFile */
                if (argv[argn][2] != 0) {
                    mapFilename = &argv[argn++][2];
                } else {
                    error("option \"m\" needs an argument");
                }
                break;
            case 'o':
                /* Output filename */
                if (argv[argn][2] != 0) {
                    outputFilename = &argv[argn][2];
                    ++argn;
                } else {
                    error("option \"o\" needs an argument");
                }
                break;
            case 's':
                /* Smart linking */
                if (argv[argn][2] != 0) {
                    smartlink = &argv[argn][2];
                    ++argn;
                } else {
                    error("option \"s\" needs an argument");
                }
                break;
            case 't':
                /* Target */
                if (targetDefined)
                    error("more than one target (option \"t\") defined");

				fprintf(stderr, "Warning: option -t is deprecated and has been replaced with -f and -c\n");
				targetDefined = true;

				string* target = str_ToLower(str_Create(&argv[argn++][2]));
				if (str_EqualConst(target, "a")) {			/* Amiga executable */
					group_SetupAmiga();
					outputFormat = FILE_FORMAT_AMIGA_EXECUTABLE;
				} else if (str_EqualConst(target, "b")) {	/* Amiga link object */
					group_SetupAmiga();
					outputFormat = FILE_FORMAT_AMIGA_LINK_OBJECT;
				} else if (str_EqualConst(target, "g")) {	/* Gameboy ROM image */
					group_SetupGameboy();
					outputFormat = FILE_FORMAT_GAMEBOY;
				} else if (str_EqualConst(target, "s")) {	/* Gameboy small mode ROM image */
					group_SetupSmallGameboy();
					outputFormat = FILE_FORMAT_GAMEBOY;
				} else if (str_EqualConst(target, "c64")) {	/* Commodore 64 .prg */
					group_SetupCommodore64();
					cbmBaseAddress = 0x0801;
					outputFormat = FILE_FORMAT_CBM_PRG;
				} else if (str_EqualConst(target, "c128")) {	/* Commodore 128 .prg */
					group_SetupUnbankedCommodore128();
					cbmBaseAddress = 0x1C01;
					outputFormat = FILE_FORMAT_CBM_PRG;
				} else if (str_EqualConst(target, "c128f")) {	/* Commodore 128 Function ROM */
					group_SetupCommodore128FunctionROM();
					outputFormat = FILE_FORMAT_BINARY;
					binaryPad = 0x8000;
				} else if (str_EqualConst(target, "c128fl")) {	/* Commodore 128 Function ROM Low */
					group_SetupCommodore128FunctionROMLow();
					outputFormat = FILE_FORMAT_BINARY;
					binaryPad = 0x4000;
				} else if (str_EqualConst(target, "c128fh")) {	/* Commodore 128 Function ROM High */
					group_SetupCommodore128FunctionROMHigh();
					outputFormat = FILE_FORMAT_BINARY;
					binaryPad = 0x4000;
				} else if (str_EqualConst(target, "c264")) {	/* Commodore 264 series .prg */
					group_SetupCommodore264();
					cbmBaseAddress = 0x1001;
					outputFormat = FILE_FORMAT_CBM_PRG;
				} else if (str_EqualConst(target, "md")) {	/* Sega Mega Drive/Genesis */
					group_SetupSegaMegaDrive();
					outputFormat = FILE_FORMAT_MEGA_DRIVE;
				} else if (str_EqualConst(target, "ms8")) {	/* Sega Master System 8 KiB */
					group_SetupSegaMasterSystem(0x2000);
					outputFormat = FILE_FORMAT_MASTER_SYSTEM;
					binaryPad = 0x2000;
				} else if (str_EqualConst(target, "ms16")) {	/* Sega Master System 16 KiB */
					group_SetupSegaMasterSystem(0x4000);
					outputFormat = FILE_FORMAT_MASTER_SYSTEM;
					binaryPad = 0x4000;
				} else if (str_EqualConst(target, "ms32")) {	/* Sega Master System 32 KiB */
					group_SetupSegaMasterSystem(0x8000);
					outputFormat = FILE_FORMAT_MASTER_SYSTEM;
					binaryPad = 0x8000;
				} else if (str_EqualConst(target, "ms48")) {	/* Sega Master System 48 KiB */
					group_SetupSegaMasterSystem(0xC000);
					outputFormat = FILE_FORMAT_MASTER_SYSTEM;
					binaryPad = 0xC000;
				} else if (str_EqualConst(target, "msb")) {	/* Sega Master System 64+ KiB */
					group_SetupSegaMasterSystemBanked();
					outputFormat = FILE_FORMAT_MASTER_SYSTEM;
					binaryPad = 0;
				} else if (str_EqualConst(target, "hc8b")) {	/* HC800 16 KiB text + data, 16 KiB bss */
					group_SetupHC8XXROM();
					outputFormat = FILE_FORMAT_HC800_KERNAL;
					binaryPad = 0;
				} else if (str_EqualConst(target, "hc8s")) {	/* HC800, CODE: 64 KiB text + data + bss */
					group_SetupHC8XXSmall();
					outputFormat = FILE_FORMAT_HC800;
					hc800Config = hc800_ConfigSmall;
				} else if (str_EqualConst(target, "hc8sh")) {	/* HC800 CODE: 64 KiB text, DATA: 64 KiB data + bss */
					group_SetupHC8XXSmallHarvard();
					outputFormat = FILE_FORMAT_HC800;
					hc800Config = hc800_ConfigSmallHarvard;
				} else if (str_EqualConst(target, "hc8m")) {	/* HC800, CODE: 32 KiB text + data + bss, CODE: 32 KiB sized banks text */
					group_SetupHC8XXMedium();
					outputFormat = FILE_FORMAT_HC800;
					hc800Config = hc800_ConfigMedium;
				} else if (str_EqualConst(target, "hc8mh")) {	/* HC800, CODE: 32 KiB text, CODE: 32 KiB sized text banks, DATA: 64 KiB data + bss */
					group_SetupHC8XXMediumHarvard();
					outputFormat = FILE_FORMAT_HC800;
					hc800Config = hc800_ConfigMediumHarvard;
				} else if (str_EqualConst(target, "hc8l")) {	/* HC800, CODE: 32 KiB text + data + bss, CODE: 32 KiB sized banks text + data + bss */
					group_SetupHC8XXLarge();
					outputFormat = FILE_FORMAT_HC800;
					hc800Config = hc800_ConfigLarge;
				} else if (str_EqualConst(target, "fxa2560x")) {	/* Foenix A2560X/K */
					group_SetupFoenixA2560X();
					outputFormat = FILE_FORMAT_TGZ;
				} else {
					error("Unknown target \"%s\"", str_String(target));
				}
                str_Free(target);
				break;

#if 0
				case 'm':
				{
					/* Use machine def file */
					targetDefined = 1;
					Error("option \"tm\" not implemented yet");
					++argn;
					break;
				}
#endif
            default:
                error("Unknown option \"%s\"", argv[argn]);
                break;
        }
    }

    if (!targetDefined) {
        error("No target format defined");
    }

    while (argn < argc && argv[argn]) {
        obj_Read(argv[argn++]);
    }

    smart_Process(smartlink);

    if (!format_SupportsReloc(outputFormat))
        assign_Process();

    patch_Process(format_SupportsReloc(outputFormat), format_SupportsOnlySectionRelativeReloc(outputFormat),
                  format_SupportsImports(outputFormat));

    if (outputFilename != NULL) {
        switch (outputFormat) {
            case FILE_FORMAT_MEGA_DRIVE:
                sega_WriteMegaDriveImage(outputFilename);
                break;
            case FILE_FORMAT_MASTER_SYSTEM:
                sega_WriteMasterSystemImage(outputFilename, binaryPad);
                break;
            case FILE_FORMAT_GAMEBOY:
                gameboy_WriteImage(outputFilename);
                break;
            case FILE_FORMAT_BINARY:
                image_WriteBinary(outputFilename, binaryPad);
                break;
            case FILE_FORMAT_CBM_PRG:
                commodore_WritePrg(outputFilename, cbmBaseAddress);
                break;
            case FILE_FORMAT_AMIGA_EXECUTABLE:
                amiga_WriteExecutable(outputFilename, false);
                break;
            case FILE_FORMAT_AMIGA_LINK_OBJECT:
                amiga_WriteLinkObject(outputFilename, false);
                break;
            case FILE_FORMAT_HC800_KERNAL:
                hc800_WriteKernal(outputFilename);
                break;
            case FILE_FORMAT_HC800:
                hc800_WriteExecutable(outputFilename, hc800Config);
                break;
            case FILE_FORMAT_TGZ:
                foenix_WriteExecutable(outputFilename);
                break;
        }
    }

    if (mapFilename != NULL) {
        if (!format_SupportsReloc(outputFormat)) {
            sect_ResolveUnresolved();
            sect_SortSections();
            map_Write(mapFilename);
        } else {
            error("Output format does not support producing a mapfile");
        }
    }

    return EXIT_SUCCESS;
}
