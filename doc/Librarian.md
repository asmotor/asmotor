# XLib
XLib is used to combine one or more object files into a library for easy distribution and use with XLink.

XLib provides several commands for working with libraries.

## Usage
    xlib library command [module1 [module2 [... modulen]]]

## Command a - Add/replace modules
The ```a``` command is used to add or replace modules of the same name in a library. If the library does not exist, it is created.

## Command d - Delete modules
The ```d``` command is used to delete the named modules from the library.

## Command l - List library contents
The ```l``` command lists all the named modules in the library.

## Command x - Extract modules
The ```x``` command extracts the named modules from the library and saves them as individual files.


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
