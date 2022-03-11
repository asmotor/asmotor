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
    TARGET_BINARY,
    TARGET_GAMEBOY,
    TARGET_AMIGA_EXECUTABLE,
    TARGET_AMIGA_LINK_OBJECT,
    TARGET_COMMODORE64_PRG,
    TARGET_COMMODORE128_PRG,
    TARGET_COMMODORE264_PRG,
    TARGET_MEGA_DRIVE,
    TARGET_MASTER_SYSTEM,
    TARGET_HC800_KERNAL,
    TARGET_HC800,
    TARGET_FOENIX_TGZ,
} TargetType;

#define HC800_HARVARD 0x01
#define HC800_32K_BANKS 0x14

static uint8_t
hc800ConfigSmall[9] = { 0x00,            
	0x81, 0x82, 0x83, 0x84, 0x81, 0x82, 0x83, 0x84 };

static uint8_t
hc800ConfigSmallHarvard[9] = { HC800_HARVARD,
	0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88 };

static uint8_t
hc800ConfigMedium[9] =	{ HC800_32K_BANKS,
	0x81, 0x82, 0x83, 0x84, 0x81, 0x82, 0x83, 0x84 };

static uint8_t
hc800ConfigMediumHarvard[9] = { HC800_HARVARD | HC800_32K_BANKS,
	0x81, 0x82, 0x87, 0x88, 0x83, 0x84, 0x85, 0x86 };

static uint8_t
hc800ConfigLarge[9] = { HC800_32K_BANKS,
	0x81, 0x82, 0x83, 0x84, 0x81, 0x82, 0x83, 0x84 };


bool
target_SupportsReloc(TargetType type) {
    return type == TARGET_AMIGA_EXECUTABLE || type == TARGET_AMIGA_LINK_OBJECT;
}

bool
target_SupportsOnlySectionRelativeReloc(TargetType type) {
    return type == TARGET_AMIGA_EXECUTABLE;
}

bool
target_SupportsImports(TargetType type) {
    return type == TARGET_AMIGA_LINK_OBJECT;
}

static void
printUsage(void) {
    printf("xlink (ASMotor v" ASMOTOR_VERSION ")\n"
           "\n"
           "Usage: xlink [options] file1 [file2 [... filen]]\n"
           "Options: (a forward slash (/) can be used instead of the dash (-))\n"
           "\t-h\t\tThis text\n"
           "\t-m<mapfile>\tWrite a mapfile\n"
           "\t-o<output>\tWrite output to file <output>\n"
           "\t-s<symbol>\tPerform smart linking starting with <symbol>\n"
           "\t-t\t\tOutput target\n"
           "\t    -ta\t\tAmiga executable\n"
           "\t    -tb\t\tAmiga link object\n"
           "\t    -tc64\tCommodore 64 .prg\n"
           "\t    -tc128\tCommodore 128 unbanked .prg\n"
           "\t    -tc128f\tCommodore 128 Function ROM (32 KiB)\n"
           "\t    -tc128fl\tCommodore 128 Function ROM Low (16 KiB)\n"
           "\t    -tc128fh\tCommodore 128 Function ROM High (16 KiB)\n"
           "\t    -tc264\tCommodore 264 series .prg\n"
           "\t    -tg\t\tGame Boy ROM image\n"
           "\t    -ts\t\tGame Boy small mode (32 KiB)\n"
           "\t    -tmd\tSega Mega Drive\n"
           "\t    -tms8\tSega Master System (8 KiB)\n"
           "\t    -tms16\tSega Master System (16 KiB)\n"
           "\t    -tms32\tSega Master System (32 KiB)\n"
           "\t    -tms48\tSega Master System (48 KiB)\n"
           "\t    -tmsb\tSega Master System banked (64+ KiB)\n"
           "\t    -thc8b\tHC800 16 KiB ROM\n"
           "\t    -thc8s\tHC800 small mode (64 KiB text + data + bss)\n"
           "\t    -thc8sh\tHC800 small Harvard mode (64 KiB text, 64 KiB data + bss)\n"
           "\t    -thc8m\tHC800 medium mode (32 KiB text + data + bss, 32 KiB sized\n"
		   "\t          \tbanks text)\n"
           "\t    -thc8mh\tHC800 medium Harvard executable (32 KiB text + 32 KiB\n"
		   "\t           \tsized text banks, 64 KiB data + bss)\n"
           "\t    -thc8l\tHC800 large mode (32 KiB text + data + bss, 32 KiB sized\n"
		   "\t          \tbanks text + data + bss)\n"
           "\t    -tfxa2560x\tFoenix A2560X/K TGZ\n"
//			"\t    -tm<mach>\tUse file <mach>\n"
    );
    exit(EXIT_SUCCESS);
}

int
main(int argc, char* argv[]) {
    int argn = 1;
    bool targetDefined = false;
    TargetType targetType = TARGET_BINARY;
	uint8_t* hc800Config = NULL;
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
                if (argv[argn][2] != 0 && !targetDefined) {
                    string* target = str_ToLower(str_Create(&argv[argn][2]));
                    if (str_EqualConst(target, "a")) {
                        /* Amiga executable */
                        group_SetupAmiga();
                        targetDefined = true;
                        targetType = TARGET_AMIGA_EXECUTABLE;
                        ++argn;
                    } else if (str_EqualConst(target, "b")) {
                        /* Amiga link object */
                        group_SetupAmiga();
                        targetDefined = true;
                        targetType = TARGET_AMIGA_LINK_OBJECT;
                        ++argn;
                    } else if (str_EqualConst(target, "g")) {
                        /* Gameboy ROM image */
                        group_SetupGameboy();
                        targetDefined = true;
                        targetType = TARGET_GAMEBOY;
                        ++argn;
                    } else if (str_EqualConst(target, "s")) {
                        /* Gameboy small mode ROM image */
                        group_SetupSmallGameboy();
                        targetDefined = true;
                        targetType = TARGET_GAMEBOY;
                        ++argn;
                    } else if (str_EqualConst(target, "c64")) {
                        /* Commodore 64 .prg */
                        group_SetupCommodore64();
                        targetDefined = true;
                        targetType = TARGET_COMMODORE64_PRG;
                        ++argn;
                    } else if (str_EqualConst(target, "c128")) {
                        /* Commodore 128 .prg */
                        group_SetupUnbankedCommodore128();
                        targetDefined = true;
                        targetType = TARGET_COMMODORE128_PRG;
                        ++argn;
                    } else if (str_EqualConst(target, "c128f")) {
                        /* Commodore 128 Function ROM */
                        group_SetupCommodore128FunctionROM();
                        targetDefined = true;
                        targetType = TARGET_BINARY;
                        binaryPad = 0x8000;
                        ++argn;
                    } else if (str_EqualConst(target, "c128fl")) {
                        /* Commodore 128 Function ROM Low */
                        group_SetupCommodore128FunctionROMLow();
                        targetDefined = true;
                        targetType = TARGET_BINARY;
                        binaryPad = 0x4000;
                        ++argn;
                    } else if (str_EqualConst(target, "c128fh")) {
                        /* Commodore 128 Function ROM High */
                        group_SetupCommodore128FunctionROMHigh();
                        targetDefined = true;
                        targetType = TARGET_BINARY;
                        binaryPad = 0x4000;
                        ++argn;
                    } else if (str_EqualConst(target, "c264")) {
                        /* Commodore 264 series .prg */
                        group_SetupCommodore264();
                        targetDefined = true;
                        targetType = TARGET_COMMODORE264_PRG;
                        ++argn;
                    } else if (str_EqualConst(target, "md")) {
                        /* Sega Mega Drive/Genesis */
                        group_SetupSegaMegaDrive();
                        targetDefined = true;
                        targetType = TARGET_MEGA_DRIVE;
                        ++argn;
                    } else if (str_EqualConst(target, "ms8")) {
                        /* Sega Master System 8 KiB */
                        group_SetupSegaMasterSystem(0x2000);
                        targetDefined = true;
                        targetType = TARGET_MASTER_SYSTEM;
                        binaryPad = 0x2000;
                        ++argn;
                    } else if (str_EqualConst(target, "ms16")) {
                        /* Sega Master System 16 KiB */
                        group_SetupSegaMasterSystem(0x4000);
                        targetDefined = true;
                        targetType = TARGET_MASTER_SYSTEM;
                        binaryPad = 0x4000;
                        ++argn;
                    } else if (str_EqualConst(target, "ms32")) {
                        /* Sega Master System 32 KiB */
                        group_SetupSegaMasterSystem(0x8000);
                        targetDefined = true;
                        targetType = TARGET_MASTER_SYSTEM;
                        binaryPad = 0x8000;
                        ++argn;
                    } else if (str_EqualConst(target, "ms48")) {
                        /* Sega Master System 48 KiB */
                        group_SetupSegaMasterSystem(0xC000);
                        targetDefined = true;
                        targetType = TARGET_MASTER_SYSTEM;
                        binaryPad = 0xC000;
                        ++argn;
                    } else if (str_EqualConst(target, "msb")) {
                        /* Sega Master System 64+ KiB */
                        group_SetupSegaMasterSystemBanked();
                        targetDefined = true;
                        targetType = TARGET_MASTER_SYSTEM;
                        binaryPad = 0;
                        ++argn;
                    } else if (str_EqualConst(target, "hc8b")) {
                        /* HC800 16 KiB text + data, 16 KiB bss */
                        group_SetupHC8XXROM();
                        targetDefined = true;
                        targetType = TARGET_HC800_KERNAL;
                        binaryPad = 0;
                        ++argn;
                    } else if (str_EqualConst(target, "hc8s")) {
                        /* HC800, CODE: 64 KiB text + data + bss */
                        group_SetupHC8XXSmall();
                        targetDefined = true;
                        targetType = TARGET_HC800;
						hc800Config = hc800ConfigSmall;
                        binaryPad = -1;
                        ++argn;
                    } else if (str_EqualConst(target, "hc8sh")) {
                        /* HC800 CODE: 64 KiB text, DATA: 64 KiB data + bss */
                        group_SetupHC8XXSmallHarvard();
                        targetDefined = true;
                        targetType = TARGET_HC800;
						hc800Config = hc800ConfigSmallHarvard;
                        binaryPad = -1;
                        ++argn;
                    } else if (str_EqualConst(target, "hc8m")) {
                        /* HC800, CODE: 32 KiB text + data + bss, CODE: 32 KiB sized banks text */
                        group_SetupHC8XXMedium();
                        targetDefined = true;
                        targetType = TARGET_HC800;
						hc800Config = hc800ConfigMedium;
                        binaryPad = -1;
                        ++argn;
                    } else if (str_EqualConst(target, "hc8mh")) {
                        /* HC800, CODE: 32 KiB text, CODE: 32 KiB sized text banks, DATA: 64 KiB data + bss */
                        group_SetupHC8XXMediumHarvard();
                        targetDefined = true;
                        targetType = TARGET_HC800;
						hc800Config = hc800ConfigMediumHarvard;
                        binaryPad = -1;
                        ++argn;
                    } else if (str_EqualConst(target, "hc8l")) {
                        /* HC800, CODE: 32 KiB text + data + bss, CODE: 32 KiB sized banks text + data + bss */
                        group_SetupHC8XXLarge();
                        targetDefined = true;
                        targetType = TARGET_HC800;
						hc800Config = hc800ConfigLarge;
                        binaryPad = -1;
                        ++argn;
                    } else if (str_EqualConst(target, "fxa2560x")) {
                        /* Foenix A2560X/K */
                        group_SetupFoenixA2560X();
                        targetDefined = true;
                        targetType = TARGET_FOENIX_TGZ;
                        binaryPad = -1;
                        ++argn;
                    } else {
                        error("Unknown target \"%s\"", str_String(target));
                    }

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
                    str_Free(target);
                } else {
                    error("more than one target (option \"t\") defined");
                }

                break;
            default:
                printf("Unknown option \"%s\", ignored\n", argv[argn]);
                ++argn;
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

    if (!target_SupportsReloc(targetType))
        assign_Process();

    patch_Process(target_SupportsReloc(targetType), target_SupportsOnlySectionRelativeReloc(targetType),
                  target_SupportsImports(targetType));

    if (outputFilename != NULL) {
        switch (targetType) {
            case TARGET_MEGA_DRIVE:
                sega_WriteMegaDriveImage(outputFilename);
                break;
            case TARGET_MASTER_SYSTEM:
                sega_WriteMasterSystemImage(outputFilename, binaryPad);
                break;
            case TARGET_GAMEBOY:
                gameboy_WriteImage(outputFilename);
                break;
            case TARGET_BINARY:
                image_WriteBinary(outputFilename, binaryPad);
                break;
            case TARGET_COMMODORE64_PRG:
                commodore_WritePrg(outputFilename, 0x0801);
                break;
            case TARGET_COMMODORE128_PRG:
                commodore_WritePrg(outputFilename, 0x1C01);
                break;
            case TARGET_COMMODORE264_PRG:
                commodore_WritePrg(outputFilename, 0x1001);
                break;
            case TARGET_AMIGA_EXECUTABLE:
                amiga_WriteExecutable(outputFilename, false);
                break;
            case TARGET_AMIGA_LINK_OBJECT:
                amiga_WriteLinkObject(outputFilename, false);
                break;
            case TARGET_HC800_KERNAL:
                hc800_WriteKernal(outputFilename);
                break;
            case TARGET_HC800:
                hc800_WriteExecutable(outputFilename, hc800Config);
                break;
            case TARGET_FOENIX_TGZ:
                foenix_WriteExecutable(outputFilename);
                break;
        }
    }

    if (mapFilename != NULL) {
        if (!target_SupportsReloc(targetType)) {
            sect_ResolveUnresolved();
            sect_SortSections();
            map_Write(mapFilename);
        } else {
            error("Output format does not support producing a mapfile");
        }
    }
        

    return EXIT_SUCCESS;
}
