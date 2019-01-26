# Invoking the assembler
Depending on the target ISA, the executable to invoke will be named motor, followed by the CPU architecture name. motorz80 (for Z80 and Game Boy), motor68k (for Motorola 680x0), motor6502 (6502 and derivatives), motormips (for MIPS), motorschip (for CHIP-8/SCHIP) or motor0x10c (for 0x010C).

## Command line options
```
-b<AS>  Change the two characters used for binary constants
        (default is 01)
-e(l|b) Change endianness
-f<f>   Output format, one of
            x - xobj (default)
            b - binary file
            v - verilog readmemh file
            g - Amiga executable file
            h - Amiga object file
-i<dir> Extra include path (can appear more than once)
-o<f>   Write assembly output to <file>
-v      Verbose text output
-w<d>   Disable warning <d> (four digits)
-z<XX>  Set the byte value (hex format) used for uninitialised
        data (default is FF) 
```

An assembler for a particular ISA may support additional options relevant for the target architecture. Please consult the documentation for the various ISA's for ISA specific options.

# Syntax
The syntax is line based. A valid line consists of an optional label, an ISA instruction or assembler instruction and an optional comment. Labels are described in the "Labels" section, assembler instructions in [The macro language](TheMacroLanguage.md).

## Comments
Comments are ignored during assembling. They are an important part of writing code, this is especially true for assembly where comments are essential for documenting what a function does - it's not as immediately obvious as when code is written in a higher level language.

Comments usually start with a semi-colon and end at the end of the line. A comment may also started with a whitespace character (including a line break) followed by an asterisk.

```
* These are comment examples
Label:  moveq   #1,d0   ;load register d0 with the value 1
        move.l  d0,d1   *copy it to d1
```

## Labels
One of the assembler's main tasks is to keep track of addresses so you don't have to remember obscure numbers but can use a meaningful name instead - a label. Labels are always placed at the beginning of a line.

Labels end with zero, one or two colons. If the label ends with two colons it will be automatically exported

Symbols and labels are always case-sensitive.

### Global labels
Global labels start with a character from A to Z (or their lower case equivalents) or an underscore. After that the characters a-z, A-Z, 0-9, _, @ and # may be used. 

A global label also marks the beginning of a new scope for local labels.

```
GlobalLabel
AnotherGlobalLabel:
ExportedLabel::
```

### Local labels
The assembler supports local labels. Local labels start with the . character, followed by a character in the range a-z, A-Z or an underscore, after which the characters a-z, A-Z, 0-9, _, @ and # may be used. Alternatively a local label may simply be a decimal integer followed by the character $.

A local label is considered local to the scope in which it is defined, a scope begins with a global label and ends with the next global label.

Local labels can only be referenced within the scope they are defined.

```
GlobalLabel:
.locallabel
42$                 ; also a local label
AnotherGlobalLabel:
```

### Exporting and importing labels

Most of the time programs consists of several source files that are assembled individually and the resulting object files then linked into an executable. This improves the time spent assembling and help manage a project.

To export a symbol (to let other source files use the symbol,) you use the keyword ```EXPORT``` (or its synonym ```XDEF```) followed by the symbol that should be exported. To import a symbol (to make an externally defined symbol available in the current source file,) the keyword ```IMPORT``` (or its synonym ```XREF```) is used.

Instead of using ```EXPORT``` or ```XDEF``` to export a label, the label may end with two colons.

Often you will want to make a header file for these definitions that other source files can include for easy access to the symbols you have exported. However, if you want to include the file in the file defining the labels, for instance if it contains structure definitions, it becomes slightly complicated - if a symbol is first imported in the include file and later appears as a label, you have multiple definitions of the symbol.

The assembler provides a third keyword, ```GLOBAL```, that either imports or exports a symbol, depending on whether it appears as a global label in the current source file.

```
EXPORT        GlobalLabel
XREF          ImportedLabel
GLOBAL        AnotherImport,AnExportedLabel
GlobalLabel:
AnExportedLabel:
AutomaticExport::
```

## Integer symbols
Instead of hardcoding constants it's often better to give them a name. The assembler supports two kinds of integer symbols, one that is constant and one that may change its value during assembling. The assembler instruction ```EQU``` is used to define constants and ```SET``` is used for variables. Instead of ```SET``` it's also possible use ```=```. Note that an integer symbol is never followed by a colon.

```
INTF_MASTER EQU $4000
MyCounter   SET 0
MyCounter   =   MyCounter+1 ;Increment MyCounter
```

### RS symbols
Integer symbols are often used to define the offsets of structure members. While the ```EQU``` instruction can be used for this it quickly becomes cumbersome when adding, reordering or removing members from the structure. The assembler provides a group of instructions to make this easier, the ```RS``` group of instructions.

| Command | Meaning |
|---|---|
| ```RSRESET``` | Resets the ```__RS``` counter to zero |
| ```RSSET constexpr``` | Sets the ```__RS``` counter to the value of constexpr |
| ```Symbol: RB constexpr``` | Sets ```Symbol``` to ```__RS``` and adds ```constexpr``` to ```__RS``` |
| ```Symbol: RW constexpr``` | Sets ```Symbol``` to ```__RS``` and adds 2*```constexpr``` to ```__RS``` |
| ```Symbol: RL constexpr``` | Sets Symbol to ```__RS``` and adds 4*```constexpr``` to ```__RS``` |

Example:
```
           RSRESET
str_pStuff RW 1
str_tData  RB 256
str_bCount RB 1
str_SIZEOF RB 0
````

Result:
| Symbol | Value |
|---|---|
| ```str_pStuff``` | 0 |
| ```str_tData``` | 2 |
| ```str_bCount``` | 258 |
| ```str_SIZEOF``` | 259 |

Like labels, constants can also be exported - if the chosen object format supports it.

## String symbols
String symbols are used to assign a name to an often used string. These symbols are expanded to their value whenever the assembler encounters the assigned name.

Example:
```
COUNTREG EQUS "[hl+]"
         ld   a,COUNTREG
```

Interpreted as:
```
         ld   a,[hl+]
```

String symbols can also be used to define small macros:
```
PUSHA    EQUS "push af\npush bc\npush de\npush hl\n"
```

## Predeclared symbols
The assembler declares several symbols:

| Name | Contents | Type |
|---|---|---|
| @, * | Current PC value | EQU |
| __RS | __RS counter | SET |
| __NARG | Number of arguments passed to macro | EQU |
| __LINE | The current line number | EQU |
| __FILE | The current filename | EQUS |
| __DATE | Todays date | EQUS |
| __TIME | The current time | EQUS |

