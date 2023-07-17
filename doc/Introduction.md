# Introduction

ASMotor is a portable and generic assembler engine and development system written in ANSI C and licensed under the GNU Public License v3. The package consists of a number of ISA specific assemblers, the librarian and the linker. The package can be used as either a cross or native development system.

ASMotor first saw the light of day as RGBDS, a Nintendo Game Boy development system. RGBDS used flex and bison and used two pass assembling. Since then it has been rewritten and now features a custom lexer and parser and the assembler does its work in only one pass, all of which make it much faster than the first versions.

Compatibility with RGBDS is not a design goal, inconsistencies and warts have been improved, but sources are largely compatible. ASMotor is heavily inspired by the Motorola 68k assembler syntax, and compatibility with sources written for the Amiga is prioritised. There were many different and incompatible assemblers released for the Amiga, thus not everything is guaranteed to work, but assembler include files released by Commodore are specifically supported by ASMotor.

If you have ever used a Motorola syntax based assembler you will feel right at home.

# Features

## CPU architectures
* Z80, Game Boy
* Motorola 680x0
* MOS 6502
* MIPS32
* CHIP-8/SCHIP
* DCPU-16
* RC811

## Assembler output formats
* xobj (ASMotor generic format)
* ELF
* flat binary
* Amiga linker object
* Amiga executable
* Verilog readmemh format

## Linker output formats
* Amiga executable
* Amiga linker object
* Commodore
    * C64 program
    * C128 program
    * C128 function ROM
    * TED (C16/C116/+4) program
* Game Boy (32 KiB ROM or banked ROM)
* Sega Mega Drive/Genesis
* Sega Master System (8 KiB/16 KiB/32 KiB/48 KiB/banked)
* Foenix PGZ
* HC800 executable
* HC800 kernel ROM

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
