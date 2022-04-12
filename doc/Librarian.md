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
* [Introduction](doc/Introduction.md), goals and background
* [Invoking the assembler](doc/Assembler.md) and basic syntax
* [Symbols](doc/Symbols.md) and labels
* [Control structures](doc/ControlStructures.md) like ```INCLUDE```, ```MACRO```'s and conditional assembling.
* [Expressions](doc/Expressions.md) and how they're built
* [Printing diagnostic messages](doc/Diagnostics.md), warnings and errors
* [Organising code](doc/OrganisingCode.md) into sections. How to define data.
* [The linker](doc/Linker.md)

# Index and reference
* [CPU specific](doc/CpuSpecifics.md) details
* [Index of all directives](doc/IndexDirectives.md)
* [Index of all functions](doc/IndexFunctions.md)
* [Operator reference](doc/ReferenceOperators.md)
* [String member reference](doc/ReferenceStringMembers.md)
