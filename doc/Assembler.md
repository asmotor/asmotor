# Invoking the assembler
Depending on the target ISA, the executable to invoke will be named motor, followed by the CPU architecture name. motorz80 (for Z80 and Game Boy), motor6809 (for Motorola 6809), motor68k (for Motorola 680x0), motor6502 (6502 and derivatives, and 65C816), motormips (for MIPS), motorschip (for CHIP-8/SCHIP) or motordcpu16 (for DCPU-16).

## Command line options
```
-a<n>    Section alignment when writing binary file
-b<AS>   Change the two characters used for binary constants
         (default is 01)
-d<FILE> Output dependency file for GNU Make
-D<NAME> Define EQU symbol with the value 1
-e(l|b)  Change endianness
-f<f>    Output format, one of
             x - xobj (default)
             e - ELF object file
             b - binary file
             v - verilog readmemh file
             g - Amiga executable file
             h - Amiga object file
-i<dir>  Extra include path (can appear more than once)
-o<f>    Write assembly output to <file>
-v       Verbose text output
-w<d>    Disable warning <d> (four digits)
-z<XX>   Set the byte value (hex format) used for uninitialised
         data (default is FF) 
```

An assembler for a particular ISA may support additional options relevant for the target architecture. Please consult the [CPU specific documentation](CpuSpecifics.md) for ISA specific options.

## <a name="setting_options"></a> Settings options in source

The ```OPT``` directive can be used to set options while assembling. The options that can be set are the
same as the command line options. The current set of options can be saved on an option stack using the
```PUSHO``` directive and restored at a later point using ```POPO```.

```
    PUSHO
    OPT w0001   ; disable warning 0001

    ; code

    POPO
```

# Syntax
The syntax is line based. A valid line consists of an optional [label](Symbols.md), an ISA instruction or assembler instruction and an optional comment. Labels are described in the "Labels" section, assembler instructions in [The macro language](TheMacroLanguage.md).

```
Label:  INSTR  ; comment
```

## Case sensitivity
Labels and other symbols are case sensitive. Directives, register names and mnemonics are not.

## Comments
Comments are ignored during assembling. They are an important part of writing code, this is especially true for assembly where comments are essential for documenting what a function does - it's not as immediately obvious as when code is written in a higher level language.

Comments usually start with a semi-colon and end at the end of the line. A comment may also begin with a whitespace character (including a line break) followed by an asterisk.

```
* These are comment examples
        moveq   #1,d0   ;load register d0 with the value 1
        move.l  d0,d1   *copy it to d1
```

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
