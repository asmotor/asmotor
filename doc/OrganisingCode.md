# Code, data and variables
From the assembler's point of view, there is no difference between code and data. Code is entered by using the mnemonics described in the relevant backend chapters.

```
    mfhi t0
    nop
    nop
    mult t0,t1
```

## <a name="data"></a> Data
Data (or instructions) can also be entered using data declaration statements:

```
    DC.W $4E75
    DC.B "This is a string",0
```

The data declaration statements will be called something different depending on the CPU backend. Portable versions are always available.

| Portable | 6502 | Z80 | M68K | MIPS | 0x10c | SCHIP |
|---|---|---|---|---|---|---|
| ```__DCB``` | ```DB``` | ```DB``` | ```DS.B``` | ```DB``` | n/a | ```DB``` |
| ```__DCW``` | ```DW``` | ```DW``` | ```DS.W``` | ```DH``` | ```DW``` | ```DW``` |
| ```__DCL``` | n/a | n/a | ```DS.L``` | ```DW``` | ```DL``` | n/a |

### <a name="binary"></a> Binary files
Data can also be declared by simply including a binary file directly from the file system:

```
        SECTION "Graphics",DATA
Ship:   INCBIN "Spaceship.bin"
```

## <a name="sections"></a> Sections

Code, data and variables are organised in sections. Before any mnemonics or data declarations can be used, a section must be declared.

```
    SECTION "A_Code_Section",CODE
```

This will switch to the section ```A_Code_Section``` if it is already known (and its type matches), or declare it if it doesn't. ```DATA``` may also be used instead of ```CODE```, they are synonymous.

### <a name="alignment"></a> Alignment

Sections are naturally aligned to a CPU specific byte alignment:

| 6502 | Z80 | M68K | MIPS | 0x10C | SCHIP |
|---|---|---|---|---|---|
| 1 | 1 | 8 | 8 | 2 | 1 |

#### CNOP

Using the CNOP directive, it's possible to align to an alignment that divides evenly into the platform's section alignment. CNOP takes two arguments - an ```offset``` and an ```alignment```. First the current PC is aligned to a multiple of ```alignment``` bytes, then ```offset``` bytes is added.

#### EVEN

Commonly used on M68K assembler, the assembler supports the EVEN directive. This does the same as ```CNOP 0,2``` - it aligns the PC to the next even address.

### <a name="space"></a> Reserving space

Variables are usually placed in a ```BSS``` section. A ```BSS``` section cannot contain initialised data, typically only the ```DS``` command is used. However, it is also possible to use the ```DB```, ```DW``` and ```DL``` commands without any arguments.

```
        SECTION "Variables",BSS
Foo:    DB            ; Reserve one byte for Foo
Bar:	DW            ; Reserve a word for Bar
Baz:	DS str_SIZEOF ; Reserve str_SIZEOF bytes for Baz```
```

If the chosen object output format supports it, you can force a section into a specific address:

```
        SECTION "LoadSection",CODE[$F000]
Code:   xor a
```

### Banks

If the target architecture supports banks, it's possible to specify that a section should be placed in 
a specific bank. Otherwise the linker will choose.

```
        SECTION "FixedSection",DATA[$1100],BANK[3]
```

It's also possible to just specify the bank:

```
        SECTION "FixedSection",CODE,BANK[3]
```

To discover the bank in which a specific symbol has been placed, the BANK() function can be used:

```
        ld  a,BANK(Symbol)
```

### <a name="origin_address"></a> Setting the origin address

At any point the origin address can be changed using the ```ORG``` directive. The origin address is the base
address for any subsequent labels and code. Note that this is different from the load address specified
using the ```SECTION``` directive. ```ORG``` is particularly useful for blocks of code that must be copied
to a specific address at a later time.

```
        SECTION "Example",CODE[$2000]

Entry:    nop                ; placed at $2000, Entry = $2000
                             ; placed at $2001

Block:                       ; Block = $2001

          ORG $1000
BlockEntry:                  ; BlockEntry = $1000
          ld  hl,BlockEntry  ; placed at $2001, load $1000 into hl
BlockSize EQU *-BlockEntry
```

## <a name="section_stack"></a> The section stack

A section stack is available, which is particularly useful when defining sections in included files (or macros) and it's necessary to preserve the section context for the program that included the file or called the macro. 

```POPS``` and ```PUSHS``` provide the interface to the section stack. ```PUSHS``` will push the current section context on the section stack. ```POPS``` can then later be used to restore it. 