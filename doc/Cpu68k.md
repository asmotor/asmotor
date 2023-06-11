# Motorola 680x0

## Register tracking

The M68K assembler will keep track of the registers that operations have modified. This is tracked in a variable named `__USED_REGMASK` where each bit corresponds to a register (bit 15 is A7 down to bit 0 which is D0). The current register mask can be reset with the `REGMASKRESET`. Additional registers can be added to `__USED_REGMASK` using the `REGMASKADD` directive.

## Instruction extensions

The `MOVEM` instruction may receive a bitmask literal instead of a register list

```
	movem.l	(sp)+,#$000F	; pop d0-d3
```

## Section alignment
Sections are aligned to a multiple of 8 bytes. 

## Command line
### -mc\<x> option
This option is used to select the CPU type. The default is 68000.

| x | CPU type |
|---|---|
| 0 | 68000 (default) |
| 1 | 68010 |
| 2 | 68020 |
| 3 | 68030 |
| 4 | 68040 |
| 6 | 68060 |

## __DCB, __RS, __DSB

|| 8 bit | 16 bit | 32 bit |
|---|---|---|---|
| Define data | DC.B | DC.W | DC.L |
| Define space | DS.B | DS.W | DS.L |
| Reserve symbol | RS.B | RS.W | RS.L |

## Section types
The Amiga has two types of memory - "chip" memory and non-chip (sometimes called "fast" memory). The assembler supports three additional section types that will place the section into chip memory. These are ```CODE_C```, ```DATA_C``` and ```BSS_C```, which correspond to ```CODE```, ```DATA``` and ```BSS``` respectively.

## Functions
The M68K assembler supports an additional function that may be used in expressions.

| Function | Meaning |
|---|---|
| regmask | [Bit mask](Expressions.md#m68k) of register list |

## Directives

| Directive | Meaning |
|---|---|
| mc68000 | Enable only 68000 instructions |
| mc68010 | Enable only 68010 instructions |
| mc68020 | Enable only 68020 instructions |
| mc68030 | Enable only 68030 instructions |
| mc68040 | Enable only 68040 instructions |
| mc68060 | Enable only 68060 instructions |
| regmaskreset | Reset the currently tracked registers in `__USED_REGMASK` |
| regmaskadd | Add a register bitmask to the tracked registers in `__USED_REGMASK` |


