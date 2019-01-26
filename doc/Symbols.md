# Symbols
The assembler supports many different kinds of symbols. Symbols are used to refer to memory addresses, constants, strings and macros.

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

Note that string symbols cannot be used in place of a string literal directly - their value is parsed by the assembler. To insert a string symbol as a string, enclose it in curly brackets:

```
    DC.B {StringSymbol},0
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

