# Directives reference

| Directive | Used for |
|---|---|
| ```MACRO```, ```ENDM```, ```SHIFT```, ```MEXIT``` | Define [macros](ControlStructures.md#macros) |
| ```REPT```, ```ENDR```, ```REXIT``` | Repeat blocks |
| ```EQU```, ```SET``` | Define [integer symbols](Symbols.md#integer_symbols) |
| ```EQUS``` | Define [string symbols](Symbols.md#string_symbols) |
| ```GROUP``` | Define a new [section group symbol](Symbols.md#group_symbols) |
| ```RSRESET```, ```RSSET```, ```__RB```, ```__RW```, ```__RL``` | Define [```RS``` symbols](Symbols.md#rs_symbols) |
| ```PRINTT```, ```PRINTV```, ```PRINTF``` | [Output text](Diagnostics.md) to stdout |
| ```FAIL```, ```WARN``` | [Display a warning or error](Diagnostics.md) on the console |
| ```EXPORT```, ```IMPORT```, ```GLOBAL```, ```XDEF```, ```XREF``` | [Export and import](Symbols.md#import_export) symbols to/from other modules |
| ```PURGE``` | [Remove a symbol](Symbols.md#purge) from the symbol table |
| ```INCLUDE``` | [Include](ControlStructures.md) and process a different source file |
| ```__DCB```, ```__DCW```, ```__DCL``` | [Define data](OrganisingCode.md#data) |
| ```INCBIN``` | Insert a [binary file](OrganisingCode.md#binary) |
| ```__DSB```, ```__DSW```, ```__DSL``` | [Reserve space](OrganisingCode.md#space) in memory |
| ```EVEN```, ```CNOP``` | Section [alignment](OrganisingCode.md#alignment) |
| ```SECTION``` | Define a new [section](OrganisingCode.md#sections) |
| ```PUSHS```, ```POPS``` | Push or pop a section on the [section stack](OrganisingCode.md#section_stack) |
| ```ORG``` | Set the [origin address](OrganisingCode.md#origin_address) for the following code |
| ```OPT``` | Set [options](Assembler.md#setting_options) |
| ```PUSHO```, ```POPO``` | [Push or pop options](Assembler.md#setting_options) on the option stack |
| ```IFC```, ```IFNC```, ```IFD```, ```IFND```, ```IF```, ```IFEQ```, ```IFGT```, ```IFGE```, ```IFLT```, ```IFLE```, ```ELSE```, ```ENDC``` | Used to form [conditional assembling](ControlStructures.md#if) blocks |
