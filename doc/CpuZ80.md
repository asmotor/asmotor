# Zilog Z80 and Game Boy
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

Synthesized instructions are safe sequences with no unintended side effects. These can be used to improve readabilility of source code.

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

