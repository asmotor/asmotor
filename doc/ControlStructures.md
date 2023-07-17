# Control structures
The assembler provides several mechanisms to transfer control of the assembly process.

## Including files

The ```INCLUDE``` command is used to process another assembly file and then return to the current file when done. If the file is not found in the current directory, the include path list will be searched. Include files may include other files.

```
INCLUDE "irq.inc"
```

## Repeating blocks

To repeat a block it can be placed inside a ```REPT```/```ENDR``` structure. The ```REPT``` construct repeats the block a specified number of times.

```
REPT 4
    add  a,c
ENDR
```

```REPT``` can also be used to repeat macro language constructs such as ```IF```/```ENDC``` and used to construct various tables.

## <a name="if"></a> Conditional assembling

The ```IF```/```ELSE```/```ENDC``` commands are used to conditionally include or exclude parts of an assembly file.

```
IF 2+2==4
    PRINTT "2+2==4\n"
ELSE
    PRINTT "2+2<>4\n"
ENDC
```

The ```ELSE``` block is optional.

### IF commands
While the integer operators can be used to test for many conditions, the assembler also supports the traditional IF commands:

| Command | True when |
|---|---|
| ```IFC s1,s2``` | The string s1 equals the string s2 |
| ```IFNC s1,s2``` | The string s1 is different from the string s2 |
| ```IFD symbol``` | The symbol symbol is defined |
| ```IFND symbol``` | The symbol symbol is not defined |
| ```IFEQ n``` | n equals zero |
| ```IFNE n``` | n is not equal to zero |
| ```IFGE n``` | n is greater than or equal to zero |
| ```IFGT n``` | n is greater than zero |
| ```IFLE n``` | n is less than or equal to zero |
| ```IFLT n``` | n is less than zero |

## <a name="macros"></a>Macros
One of the most useful features of an assembler is the ability to write macros. Macros provide a method of passing arguments to them and they can then react to the input using conditional assembling constructs.

Macros cannot be exported or imported.

Macros are defined by a symbol name, followed by a colon and then the macro definition enclosed in ```MACRO```/```ENDM```:

```
MyMacro: MACRO
         ld   a,80
         call MyFunc
         ENDM
```

The example above is a very simple macro. You use the macro by using its name where you would normally use an instruction.

```
         add a,b
         ld  sp,hl
         MyMacro  ;This will be expanded
         sub a,87
```

When the assembler sees ```MyMacro``` it will insert the macro definition, the text enclosed in ```MACRO```/```ENDM```.

### Macro loops
Often macros will contains loops, such as:

```
LoopMacro: MACRO
           xor a,a
.loop      ld  [hl+],a
           dec c
           jr  nz,.loop
           ENDM
```

This will work fine, until you start using the macro more than once per scope. The macro defines the symbol ```.loop```, which the assembler will attempt to define every time it is called. To get around this problem there is a special label string symbol called ```\@``` that you can append to your labels and it will then expand to a unique string. ```\@``` also works in ```REPT```-blocks.

```
LoopMacro: MACRO
           xor a,a
.loop\@    ld  [hl+],a
           dec c
           jr  nz,.loop\@
           ENDM
```

### Arguments

```LoopMacro``` above could be improved - it would be better if the user didn't have to preload the registers with values and then call the macro. Fortunately it's possible to pass arguments to a macro, the ```LoopMacro``` example would then be able to load the registers itself.

Arguments are passed verbatim, separated by commas.

In macros you can get the arguments by using the special macro string equates ```\1``` through ```\9```, ```\1``` being the first argument specified on the call of the macro.

```
LoopMacro: MACRO
           ld  hl,\1
           ld  c,\2
           xor a,a
.loop\@    ld  [hl+],a
           dec c
           jr  nz,.loop\@
           ENDM
```

The macro now accepts two arguments. The first being an address and the second being a byte count. The macro will then set all bytes in this range to zero.

```
LoopMacro MyVars,54
```

You can specify up to nine arguments when calling a macro. Arguments are passed as string equates, there's no need to enclose them in quotes. As the arguments are inserted verbatim, it's a good idea to use brackets around ```\1```-```\9``` if you perform further calculations on them. Consider the following scenario:

```
PrintValue: MACRO
            PRINTV \1*2
            ENDM

            PrintValue 1+2
```

The assembler will print the value 5 on screen and not 6 as you might expect. The solution is to enclose ```\1``` in brackets:

```
PrintValue: MACRO
            PRINTV (\1)*2
            ENDM
```

Sometimes it may be necessary to pass a comma into a macro. To do this, the macro argument can be enclosed in angle brackets:

```
PrintString: MACRO
             lea  .string\@(PC),a0
             jsr  _print
             bra  .skip\@
.string\@    dc.b \1
             EVEN
.skip\@
             ENDM

             PrintString <"Hello,"," world",0>
```

### The special argument \0

Particularly useful on Motorola 680x0, it's also possible to use the special argument ```\0```. To pass a value into ```\0``` you append a dot (.) followed by the value to the macro name.

```
push: MACRO
      movem.\0 \1,-(sp)
      ENDM

      push.l   d0-a6
```

### SHIFT
```SHIFT``` is a command only available in macros and particularly useful in ```REPT```-blocks. It shifts the macro arguments one position "to the left" - ```\1``` will get ```\2```'s value, ```\2``` will get ```\3```'s value and so forth.

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
