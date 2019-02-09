# CPU specific details

# 6502

## Command line

### -mu\<x> option
Some versions of the 6502 series have a number of undocumented instructions. As they are undocumented, no official mnemonics exist. Several sets of mnemonics are in use, however. These can be selected with the ```-mu<x>``` option, where x is 0, 1 or 2.

| Set #0 | Set #1 | Set #2 |
|---|---|---|
| AAC | ANC | ANC |
| AAX | SAX | AXS |
| ARR | ARR | ARR |
| ASR | ASR | ALR |
| ATX | LXA | OAL |
| AXA | SHA | AXA |
| AXS | SBX | SAX |
| DCP | DCP | DCM |
| DOP | DOP | SKB |
| ISC | ISB | INS |
| KIL | JAM | HLT |
| LAR | LAE | LAS |
| LAX | LAX | LAX |
| RLA | RLA | RLA |
| RRA | RRA | RRA |
| SLO | SLO | ASO |
| SRE | SRE | LSE |
| SXA | SHX | XAS |
| SYA | SHY | SAY |
| TOP | TOP | SKW |
| XAA | ANE | XAA |
| XAS | SHS | TAS |

## __DCB, __RS, __DSB

|| 8 bit | 16 bit | 32 bit |
|---|---|---|---|
| Define data | DB | DW | |
| Define space | DS | | |
| Reserve symbol | RB | RW | |

# Z80
## Numeric formats
The Z80 backend supports an additional numeric literal format. The first character is the backtick:

```
`00112233
```

The values are pixel values. The values are converted from chunky data format to the planar format used by the Game Boy.

## Command line
### -mg\<ASCI> option
The ```-mg``` option can be used to change the four characters used for Game Boy graphic literals.

### -mc\<x> option
This option is used to select the CPU type. The default is Z80.

| x | CPU type |
|---|---|
| g | Gameboy |
| z | Z80 (default) |

## __DCB, __RS, __DSB

|| 8 bit | 16 bit | 32 bit |
|---|---|---|---|
| Define data | DB | DW | |
| Define space | DS | | |
| Reserve symbol | RB | RW | |

# MIPS
## Command line
### -mc\<x>  option
This option is used to select the CPU type. The default is MIPS32 R2

| x | CPU type |
|---|---|
| 0 | MIPS32 R1 |
| 1 | MIPS32 R2 (default) |

## __DCB, __RS, __DSB

|| 8 bit | 16 bit | 32 bit |
|---|---|---|---|
| Define data | DB | DH | DW |
| Define space | DSB | DSH | DSW |
| Reserve symbol | RB | RH | RW |

# M68K
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
The Amiga has two types of memory - "chip" memory and non-chip (sometimes called "fast" memory). The assembler supports three additional section types, that will placed the section into chip memory. These are ```CODE_C```, ```DATA_C``` and ```BSS_C```, which correspond to ```CODE```, ```DATA``` and ```BSS``` respectively.

## Functions
The M68K assembler supports an additional function that may be used in expressions.

| Function | Meaning |
|---|---|
| regmask | [Bit mask](Expressions.md#m68k) of register list |

# SCHIP
## Registers
## Mnemonics


## Command line
### -mc\<x> option
This option is used to select the CPU type. The default is SCHIP.

| x | CPU type |
|---|---|
| 0 | CHIP-8 |
| 1 | SCHIP (default) |

## __DCB, __RS, __DSB
|| 8 bit | 16 bit | 32 bit |
|---|---|---|---|
| Define data | DB | DW | |
| Define space | DSB | DSW | |
| Reserve symbol | RB | RW | |


# 0x10C
## Word size
## Command line
## __DCB, __RS, __DSB


SECTION ALIGNMENT