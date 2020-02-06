# CPU specific details

# 6502

## Section alignment
Sections are not aligned to any particular multiple.

## Command line

### -mu\<x> option
Some versions of the 6502 series have a number of undocumented instructions. As they are undocumented, no official mnemonics exist. Several sets of mnemonics are in use, however. These can be selected with the ```-mu<x>``` option, where x is 0 (no undocumented opcodes), 1, 2 or 3.

| Set #1 | Set #2 | Set #3 |
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
## Section alignment
Sections are not aligned to any particular multiple.

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

### -ms<x> option
This option enables certain synthesized instructions.

| x | Synthesized instructions |
|---|---|
| 0 | Disabled |
| 1 | Enabled |

Synthesized instructions are safe sequences with no unintended side effects. These can be used to improve
readabilility of source code.

| Mnemonic | Notes |
|---|---|
| LD reg16,reg16 | reg16 = BC, DE or HL |
| LD ireg,reg16 | ireg = IX or IY, reg16 = BC or DE - needs undocumented instructions enabled |
| LD reg16,ireg | ireg = IX or IY, reg16 = BC or DE - needs undocumented instructions enabled |
| LD reg16,(ireg+n8) | reg16 = BC, DE or HL, ireg = IX or IY |
| LD (ireg+n8),reg16 | reg16 = BC, DE or HL, ireg = IX or IY |
| SLA reg16 | reg16 = BC, DE or HL |
| SLL reg16 | reg16 = BC, DE or HL |
| SRA reg16 | reg16 = BC, DE or HL |
| SRL reg16 | reg16 = BC, DE or HL |

## __DCB, __RS, __DSB

|| 8 bit | 16 bit | 32 bit |
|---|---|---|---|
| Define data | DB | DW | |
| Define space | DS | | |
| Reserve symbol | RB | RW | |

# MIPS
## Section alignment
Sections are aligned to a multiple of 8 bytes.

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
The Amiga has two types of memory - "chip" memory and non-chip (sometimes called "fast" memory). The assembler supports three additional section types, that will placed the section into chip memory. These are ```CODE_C```, ```DATA_C``` and ```BSS_C```, which correspond to ```CODE```, ```DATA``` and ```BSS``` respectively.

## Functions
The M68K assembler supports an additional function that may be used in expressions.

| Function | Meaning |
|---|---|
| regmask | [Bit mask](Expressions.md#m68k) of register list |

# SCHIP
CHIP-8 and SCHIP have no officially defined mnemonics. ASMotor uses a set of mnemonics that is similar to other ISA's and should be easy to learn.

## Section alignment
Sections are not aligned to any particular multiple.

## Registers
The registers are named ```V0``` to ```VF```. ```VA``` to ```VF``` may also be referred to as ```V10``` to ```V15```

## Mnemonics
### CHIP-8
| Opcode | Mnemonic |
|---|---|
| 00E0 | CLS |
| 00EE | RET |
| 1nnn | JP nnn |
| 2nnn | CALL nnn |
| 3xnn | SE Vx,nn |
| 4xnn | SNE Vx,nn |
| 5xy0 | SE Vx,y |
| 6xnn | LD Vx,nn |
| 7xnn | ADD Vx,nn |
| 8xy0 | LD Vx,Vy |
| 8xy1 | OR Vx,Vy |
| 8xy2 | AND Vx,Vy |
| 8xy3 | XOR Vx,Vy |
| 8xy4 | ADD Vx,Vy |
| 8xy5 | SUB Vx,Vy |
| 8xy6 | SHR Vx |
| 8xy7 | SUB Vx,Vy |
| 8xyE | SHL Vx |
| 9xy0 | SNE x,y |
| Annn | LD I,nnn |
| Bnnn | JP V0+nnn |
| Cxnn | RND Vx,nn |
| Dxyn | DRW Vx,Vy,n |
| Ex9E | SKP Vx |
| ExA1 | SKNP Vx |
| Fx07 | LD Vx,DT |
| Fx0A | WKP Vx |
| Fx15 | LD DT,Vx |
| Fx18 | LD ST,Vx |
| Fx1E | ADD I,Vx |
| Fx29 | LDF Vx |
| Fx33 | BCD Vx |
| Fx55 | LDM (I),Vx |
| Fx65 | LDM Vx,(I) |

### Additional SCHIP instructions
| Opcode | Mnemonic |
|---|---|
| 00C0 | SCD |
| 00FB | SCR |
| 00FC | SCL |
| 00FD | EXIT |
| 00FE | LO |
| 00FF | HI |
| Fx30 | LDF10 Vx |
| Fx75 | LDM RPL,Vx |
| Fx85 | LDM Vx,RPL |


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


# DCPU-16
0x10C was a game that was going to feature a user programmable fantasy CPU, the DCPU-16. ASMotor supports assembling code for this CPU.

DCPU-16 specifications can be [found here](https://gist.github.com/metaphox/3888117).

## Word size
This CPU is a little different in that its native word size is 16 bit and it's unable to address individual bytes.

## Section alignment
Sections are aligned to native 16 bit words.

## Command line
### -mo\<x>
This option controls the level of optimization. 0 means no optimization, while 1 will optimize addressing modes to use shorter forms when possible.

## __DCB, __RS, __DSB
|| 8 bit | 16 bit | 32 bit |
|---|---|---|---|
| Define data | | DW | DL |
| Define space | | DSW | |
| Reserve symbol | | RW | RL |
