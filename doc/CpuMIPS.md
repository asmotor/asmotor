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

