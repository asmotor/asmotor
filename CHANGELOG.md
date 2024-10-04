# 1.3.2

## Assembler

* Implemented `INCLUDE ONCE` functionality.
* 45GS02 has an `LDQ #n32` synthesized instruction.
* It is no longer possible to redefine an `RS` type symbol.
* Fixed crash when trying to redefine internal symbol.


## Linker

* Error message fixes


# 1.3.1

Maintenance release that fixes compilation errors with MacPorts.


# 1.3.0

Major highlights from this release are support for a number of new CPUs - 65C02, 65C816, 4510, 45GS02, and 6809. Support for Foenix A2560K/X and F256, the CoCo, and the Commodore 65 and MEGA65 har also been implemented along with various file formats.

The assembler is now also able to write 68K ELF objects, improving integration with C compilers. The linker is also able to read ELF object files.

A simple but effective new feature is using the `RS` family of commands to define structures with members, consult the documentation for more information.

The linker implements a new DSL for precise user control of memory layout, section placement and bank numbering. All of the existing, built-in machine configurations could also be implemented using the new DSL, but are retained as built-in options.

The VSCode plugin has also been upgraded, and now supports features such as go to definition, show definitions in file, and more.


## Assembler
* The `RS` family of commands now do double duty as a struct definition feature
* Backslash no longer needed to access labels in a scope
* New postfix `#` operator for forcibly inserting a string `EQUS` value and concatenating
* Symbol scope preserved with `PUSHS` and `POPS`
* ELF output option for the 68K family
* Errors properly printed to `stderr`, noise removed
* Disallow trailing comma in DB and friends
* String interpolation bugfixes
* REPT line numbering fixed
* Miscellaneous minor fixes

### 6502
* 65c02 support
* 65c816 support
* Support for 4510 (Commodore 65) and 45GS02 (MEGA65)

### 6809
* Motorola 6809 is a new addition in this release

### 680x0
* ELF object file format support
* Used registers tracking implemented through `__USED_REGMASK`. New `REGMASKRESET` and `REGMASK` directives added.
* New section groups for Foenix A2560K/X
* Bug fixes for DBcc, Bcc with short addressing and absolute positioning
* PC with zero displacement addressing mode fix

### RC8xx
* Implement latest revision and synthesized instructions
* Handle `EQUS` indirect registers

## Linker
* New machine definition DSL for specifying target memory layout and banks implemented
* New entry point (`-e`) option for output formats that support it
* `ELF` file format support for 680x0
* Support for Foenix A2560K/X and `PGZ` file format
* Foenix F256 file formats implemented
* MEGA65 output format and machine definition added
* CoCo output format
* `-t` option marked deprecated and replaced by `-c` and `-f`
* Various minor fixes


# 1.2.0
## Assembler
* `ROOT` section flag added
* Unterminated `REPT`/`IF`/`MACRO` block fixes
* Error message improvements
