# Code, data and variables
From the assembler's point of view, there is no difference between code and data. Code is entered by using the mnemonics described in the relevant backend chapters.

```
    mfhi t0
    nop
    nop
    mult t0,t1
```

Data (or instructions) can also be entered using data declaration statements:

```
    DC.W $4E75
    DC.B "This is a string",0
````

The data declaration statements may be called something different depending on the CPU backend. Please refer to the relevant backend chapter.

Data can also be declared by simply including a binary file directly from the file system:

```
        SECTION "Graphics",DATA
Ship:   INCBIN "Spaceship.bin"
````

## Sections

Code, data and variables are organised in sections. Before any mnemonics or data declarations can be used, a section must be declared.

```
    SECTION "A_Code_Section",CODE
```

This will switch to the section ```A_Code_Section``` if it is already known (and its type matches), or declare it if it doesn't. ```DATA``` may also be used instead of ```CODE```, they are synonymous.

Variables are usually placed in a ```BSS``` section. A ```BSS``` section cannot contain initialised data, typically only the ```DS``` command is used. However, it is also possible to use the ```DB```, ```DW``` and ```DL``` commands without any arguments.

```
        SECTION "Variables",BSS
Foo:    DB            ; Reserve one byte for Foo
Bar:	DW            ; Reserve a word for Bar
Baz:	DS str_SIZEOF ; Reserve str_SIZEOF bytes for Baz```
````

If the chosen object output format supports it, you can force a section into a specific address:

```
        SECTION "LoadSection",CODE[$F000]
Code:   xor a
```

The different CPU backends may support additional section types and other options. Please refer to the relevant chapters.

## The section stack

A section stack is available, which is particularly useful when defining sections in included files (or macros) and it's necessary to preserve the section context for the program that included the file or called the macro. 

```POPS``` and ```PUSHS``` provide the interface to the section stack. ```PUSHS``` will push the current section context on the section stack. ```POPS``` can then later be used to restore it. 