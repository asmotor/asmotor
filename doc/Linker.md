# XLink
The linker is used to combine one or more object files produced by the assembler into a runnable executable of some kind.

The linker will place sections according to the their placement (bank, offset) defined in the assembly file. If no particular location is defined, the order of objects on the command line matters. The first section of the first object will generally be placed first in the resulting image. This is often the entry point, but some platforms may
define other entry points.

## Linker map

```
POOL name image_offset cpu_address cpu_bank size
POOLS name [i : start..end] image_offset_expr cpu_address_expr cpu_bank_expr size_expr
GROUP pool_name_1 pool_name_2 ... pool_name_n
```

## Usage
    xlink [options] file1 [file2 [... filen]]

## Options
### Help (-h)
Prints a short summary of all options.
```
-h   Short help text
```

### Memory configuration (-c)

This option select the target machine's memory configuration. Various sections types, names, banks and locations will be defined.

```
-c<config>  Memory configuration 
```

| Configuration | Description |
|---|---|
| amiga | Amiga |
| cbm64    | Commodore 64 |
| cbm128   | Commodore 128 unbanked |
| cbm128f  | Commodore 128 Function ROM (32 KiB) |
| cbm128fl | Commodore 128 Function ROM Low (16 KiB) |
| cbm128fh | Commodore 128 Function ROM High (16 KiB) |
| cbm264   | Commodore 264/TED series |
| ngb      | Game Boy ROM image |
| ngbs     | Game Boy small mode (32 KiB) |
| smd      | Sega Mega Drive |
| sms8     | Sega Master System (8 KiB) |
| sms16    | Sega Master System (16 KiB) |
| sms32    | Sega Master System (32 KiB) |
| sms48    | Sega Master System (48 KiB) |
| smsb     | Sega Master System banked (64+ KiB) |
| hc800b   | HC800 16 KiB ROM |
| hc800s   | HC800 small mode (64 KiB text + data + bss) |
| hc800sh  | HC800 small Harvard mode (64 KiB text, 64 KiB data + bss) |
| hc800m   | HC800 medium mode (32 KiB text + data + bss, 32 KiB sized banks text |
| hc800mh  | HC800 medium Harvard executable (32 KiB text + 32 KiB sized text banks, 64 KiB data + bss) |
| hc800l   | HC800 large mode (32 KiB text + data + bss, 32 KiB sized banks text + data + bss) |
| fxa2560x | Foenix A2560X/K |

#### **Commodore 128 Function ROM (-ccbm128f, -ccbm128fl, -ccbm128fh)**

The Commodore 128 Function ROM is a binary format that can be burned onto an EPROM and placed internally in the C128 (or externally on a cartridge). The images must start with a particular header. XLink doesn't produce this header automatically.

The first object's first section must contain this header. Alternatively it's possible to specify a load address for a section and placed it at the right address.


### Output file format format
This option selects the target image format.

```
-f<target>  Output file format
```

| Target | Description |
|---|---|
| amigaexe | Amiga executable |
| amigalink | Amiga link object, for further linking with an Amiga linker |
| binary | Raw binary file |
| cbm | Commodore .prg |
| ngb | Nintendo Game Boy ROM |
| smd | Sega Mega Drive ROM |
| sms | Sega Master System ROM |
| hc800 | HC800 executable |
| hc800k | HC800 kernal ROM |
| fxpgz | Foenix PGZ |

#### **Amiga executable format (-tamigaexe)**

An Amiga executable's entry point is the first section. AmigaOS will simply JSR to the first address. The first object file's first section will thus be used. The order of objects therefore matters.

The Amiga format does not support placement of sections, nor does it support banks.

#### **Amiga link object (-tamigalink)**

This output format produces an Amiga link object that can be processed by an Amiga linker. This way it's possible to use ASMotor's advanced object format and linking expressions for assembly code, and then later link with C code, for instance.

#### **Binary (-tbinary)**

A binary file's contents is all the sections concatenated. The first byte of the file is the first byte of the first section, regardless of the section's desired placement. Subsequent sections will be placed in the file relative to the first section, introducing padding as necessary.

#### **Commodore PRG (-tcbm)**

This format's entry point is the first object's first section. XLink will generate the right SYS instruction and address for you. The first section may specify a location. If not, the lowest possible BASIC address is used.

#### **Game Boy ROM (-ngb)**

The Game Boy format is a binary format. Game Boy images must start with a particular header. The linker does not produce this header automatically, but it does calculate the correct checksum.

The first object's first section must contain this header. Alternatively it's possible to specify a load address for a section and place it at the right address.

The normal mode supports banks. The linker will distribute CODE and DATA sections, filling up the lower banks first. Sections may also specify a particular bank if necessary. 

#### **Sega Mega Drive/Genesis (-smd)**

This is a binary format. A Mega Drive image must start with a particular header. The linker does not produce this header automatically, but will update it.

The first object's first section must contain this header. Alternatively it's possible to specify a load address for a section and place it at the right address.

The linker will update the header's ROM ending address and checksum bytes accordingly.

#### **Sega Master System (-sms)**

This is a binary format. A Master System image must contain a particular header. The linker does not produce this header automatically, but will update it.

A section loaded at the right address must contain this header.

The linker will update the header's size and checksum accordingly.

### Output file (-m)

If not specified, no map file will be produced.

```
-m<output>  Write mapfile to file <output>
```

### Output file (-o)

If not specified, no output will be produced.

```
-o<output>  Write output to file <output>
```

### Strip unused sections (-s)

If not specified, no output will be produced.

```
-s<symbol>  Strip unused sections, rooting the section containing <symbol>
```

The section containing `<symbol>` will be considered in use, as will any sections it references. Sections may also have been marked as `ROOT` by the assembler, these will also be considered in use. Any section not in use will not be output to the final file.


# Further reading
* [Introduction](Introduction.md), goals and background
* [Invoking the assembler](Assembler.md) and basic syntax
* [Symbols](Symbols.md) and labels
* [Control structures](ControlStructures.md) like ```INCLUDE```, ```MACRO```'s and conditional assembling.
* [Expressions](Expressions.md) and how they're built
* [Printing diagnostic messages](Diagnostics.md), warnings and errors
* [Organising code](OrganisingCode.md) into sections. How to define data.
* [The linker](Linker.md)

# Index and reference
* [CPU specific](CpuSpecifics.md) details
* [Index of all directives](IndexDirectives.md)
* [Index of all functions](IndexFunctions.md)
* [Operator reference](ReferenceOperators.md)
* [String member reference](ReferenceStringMembers.md)
