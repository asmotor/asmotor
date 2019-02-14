# XLink
The linker is used to combine one or more object files produced by the assembler into a runnable executable of some kind.

The linker will place sections according to the their placement (bank, offset) defined in the assembly file. If no particular location is defined, the order of objects on the command line matters. The first section of the first object will generally be placed first in the resulting image. This is often the entry point, but some platforms may
define other entry points.

## Usage
    xlink [options] file1 file2 ... filen

## Options
### -h, Help
Prints a short summary of all options.
```
-h   Short help text
```

### Output file
If not specified, no output will be produced.

```
-o<output>  Write output to file <output>
```

### Output target format
This option selects the target memory layout and image format.

```
-t<target>  Output target
```

| Target | Description |
|---|---|
| a | Amiga executable |
| b | Amiga link object, for further linking with an Amiga linker |
| c64 | Commodore 64 .prg |
| c128 | Commodore 128 unbanked .prg |
| c128f | Commodore 128 Function ROM (32 KiB) |
| c128fl | Commodore 128 Function ROM Low (16 KiB) |
| c128fh | Commodore 128 Function ROM High (16 KiB) |
| c264 | Commodore 264 series (C116, C16, Plus/4) .prg |
| g | Game Boy ROM image |
| s | Game Boy small mode (32 KiB) |
| md | Sega Mega Drive |
| ms8 | Sega Master System (8 KiB) |
| ms16 | Sega Master System (16 KiB) |
| ms32 | Sega Master System (32 KiB) |
| ms48 | Sega Master System (48 KiB) |
| msb | Sega Master System banked (>=64 KiB) |

# Amiga format (-ta)
An Amiga executable's entry point is the first section. AmigaOS will simply JSR to the first address. The first object file's first section will thus be used. The order of objects therefore matters.

The Amiga format does not support placement of sections, nor does it support banks.

# Amiga link object (-tb)
This output format produces an Amiga link object that can be processed by an Amiga linker. This way it's possible to use ASMotor's advanced object format and linking expressions for assembly code, and then later link with C code, for instance.

# Commodore PRG (-tc64, -tc128, -tc264)
These formats' entry point is the first object's first section. XLink will generate the right SYS address for you. The first section may specify a location. If not, the lowest possible BASIC address is used.

# Commodore 128 Function ROM (-tc128f, -tc128fl, -tc128fh)
The Commodore 128 Function ROM is a binary format that can be burned onto an EPROM and placed internally in the C128 (or externally on a cartridge). The images must start with a particular header. XLink doesn't produce this header automatically.

The first object's first section must contain this header. Alternatively it's possible to specify a load address for a section and placed it at the right address.

# Game Boy (-tg, -ts)
The Game Boy formats are binary formats. Game Boy images must start with a particular header. The linker does not produce this header automatically.

The first object's first section must contain this header. Alternatively it's possible to specify a load address for a section and place it at the right address.

The normal mode supports banks. The linker will distribute CODE and DATA sections, filling up the lower banks first. Sections may also specify a particular bank if necessary. 

# Mega Drive/Genesis (-tmd)
This is a binary format. A Mega Drive image must start with a particular header. The linker does not produce this header automatically, but will update it.

The first object's first section must contain this header. Alternatively it's possible to specify a load address for a section and place it at the right address.

The linker will update the header's ROM ending address and checksum bytes accordingly.

# Sega Master System (-tms8, -tms16, -tms32, -tms48, -tmsb)
This is a binary format. A Master System image must contain a particular header. The linker does not produce this header automatically, but will update it.

A section loaded at the right address must contain this header.

The linker will update the header's size and checksum accordingly.
