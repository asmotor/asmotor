# XLink
The linker is used to combine one or more object files produced by the assembler into a runnable executable of some kind.

The linker will place sections according to the their placement (bank, offset) defined in the assembly file. If no particular location is defined, the order of objects on the command line matters. The first section of the first object will generally be placed first in the resulting image. This is often the entry point, but some platforms may
define other entry points.

## Usage
    xlink [options] file1 [file2 [... filen]]

## Machine definitions
In order to link object files for a specific platform, it must first be defined. Definitions for several platforms are already built in.

* [Amiga](LinkerAmiga.md)
* [Commodore 8-bit](LinkerCommodore.md)
* [Nintendo Game Boy](LinkerGameboy.md)
* [Sega Mega Drive](LinkerMegaDrive.md)
* [HC800](LinkerHC800.md)
* [Foenix A2560K/X](LinkerFoenix2560.md)
* [Foenix F256 line](LinkerFoenix256.md)

A [custom machine definition](LinkerMemoryLayoutFile.md) may also defined by the user.

## Options
### Help (-h)
Prints a short summary of all options.
```
-h   Short help text
```

### Machine definition (-a)
A machine definition file can be used to specify memory regions - locations, sizes and types.

Please refer to [machine definition file documentation](LinkerMemoryLayoutFile.md) for documentation on this format.

### Memory configuration (-c)
Several machine definitions are built-in. This option selects a particular target machine's memory configuration. Various sections types, names, banks and locations will be defined.

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
| smsb     | Sega Master System banked (64+ KiB) |
| hc8b     | HC800 16 KiB ROM |
| hc8s     | HC800 small mode (64 KiB text + data + bss) |
| hc8sh    | HC800 small Harvard mode (64 KiB text, 64 KiB data + bss) |
| hc8m     | HC800 medium mode (32 KiB text + data + bss, 32 KiB sized banks text) |
| hc8mh    | HC800 medium Harvard executable (32 KiB text + 32 KiB sized text banks, 64 KiB data + bss) |
| hc8l     | HC800 large mode (32 KiB text + data + bss, 32 KiB sized banks text + data + bss) |
| fxa2560x | Foenix A2560X/K |
| mega65   | MEGA65 unbanked  |

### Output file format format
This option selects the target image format.

```
-f <target>  Output file format
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
| hc800k | HC800 kernel ROM |
| fxpgz | Foenix PGZ |
| mega65 | MEGA65 .PRG |

#### **Binary (-tbinary)**

A binary file's contents is all the sections concatenated. The first byte of the file is the first byte of the first section, regardless of the section's desired placement. Subsequent sections will be placed in the file relative to the first section, introducing padding as necessary.

### Output file (-m)

If not specified, no map file will be produced.

```
-m <output>  Write map file to file <output>
```

The map file will include symbol definition location, when debug information is emitted by the assembler (assembler option `-g`)

### Output file (-o)

If not specified, no output will be produced.

```
-o <output>  Write output to file <output>
```

### Strip unused sections (-s)

```
-s <symbol>  Strip unused sections, rooting the section containing <symbol>
```

The section containing `<symbol>` will be considered in use, as will any sections it references. Sections may also have been marked as `ROOT` by the assembler, these will also be considered in use. Any section not in use will not be output to the final file.

## Input object formats

Besides native object files, the linker reads SDCC/sdas `RREL` text object
files (`.rel`) directly. A file whose first line starts with `[XDQ][HL]<n>`
is treated as `RREL`; everything else follows the native binary path.

Support is deliberately narrow and refuses loudly what it does not
implement — an unrecognised construct is an error, never a guess:

* Only the magic `XL2` is accepted: hexadecimal values, little-endian
  16-bit indices and offsets, two digits per stored byte. Other radixes,
  endiannesses and digit widths are refused.
* Areas are merged across modules by name, mirroring `sdld`: each module's
  contribution is appended at the base recorded in its own `A` records, and
  the linker exports the synthesised area-label globals `s_<AREA>` (area
  start) and `l_<AREA>` (area length) that `sdld` also provides, because
  sdas-generated C glue references them.
* Reserved space (bytes an area declares with its `A` size but never writes
  with a `T` record) is emitted as zero padding; `sdld`'s Intel-hex output
  simply omits those bytes.
* Relocation modes are accepted from a fixed whitelist — for the RC800
  (sdcc `-mrc8` objects, linked with the HC800 machine definition) these are
  the modes sdas actually emits: absolute word, word symbol reference, and
  the two-byte-stored-byte forms (low or high byte of the object, optionally
  PC-relative, optionally symbol-relative). Every other mode, `P`
  (bit-positioning) and `G`/`B` (bank) records, overlay or paged areas, and
  text records whose offsets wrap are refused.
* `RREL` archives (`.relf`) are not yet supported.

The reader is exercised by the `test_integration_sdrel` suite against
byte-exact `sdldrc8` oracle output.


# Further reading
* [Introduction](Introduction.md), goals and background
* [Invoking the assembler](Assembler.md) and basic syntax
* [Symbols](Symbols.md) and labels
* [Control structures](ControlStructures.md) like ```INCLUDE```, ```MACRO```s and conditional assembling.
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
