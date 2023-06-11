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


