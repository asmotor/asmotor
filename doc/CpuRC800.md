# RC800

The RC800 is the CPU of the HC800 computer. This document covers assembler-specific behavior.

## Section alignment
Sections are not aligned to any particular multiple.

## Endianness
Multi-byte values are emitted least significant byte first by default.

## Command line

### -ms\<x> option
This option enables certain synthesized instructions.

| x | Synthesized instructions |
|---|---|
| 0 | Disabled |
| 1 | Enabled (default) |

Synthesized instructions are safe sequences with no unintended side effects. These can be used to improve readability of source code.

## Synthesized instructions

| Mnemonic | Expansion |
|---|---|
| ADD R16,imm | `ADD R16,n8` if the value fits in -128..127, otherwise `ADD R16,low` / `ADD F,high` (omitting zero parts) |
| SUB R,imm | `ADD R,-imm` |
| LD R16,imm | Two 8 bit loads, with a 3 byte optimization: `EXT` if the value is sign-extendable, `LD F,T` if both bytes are equal |
| LD FT,(R16) | `LD T,(R16)` / `ADD R16,1` / `EXG F,T` / `LD T,(R16)` / `ADD R16,-1` / `EXG F,T` |
| LD (R16),FT | `LD (R16),T` / `ADD R16,1` / `EXG F,T` / `LD (R16),T` / `ADD R16,-1` / `EXG F,T` |
| LD FT,(R16+) | `LD T,(R16)` / `ADD R16,1` / `EXG F,T` / `LD T,(R16)` / `EXG F,T` |
| LD (R16+),FT | `LD (R16),T` / `ADD R16,1` / `EXG F,T` / `LD (R16),T` / `EXG F,T` |
| LD FT,(-R16) | `LD T,(R16)` / `ADD R16,-1` / `EXG F,T` / `LD T,(R16)` |
| LD (-R16),FT | `EXG F,T` / `LD (R16),T` / `ADD R16,-1` / `EXG F,T` / `LD (R16),T` |
| LD T,(R16+) | `LD T,(R16)` / `ADD R16,1` |
| LD (R16+),T | `LD (R16),T` / `ADD R16,1` |
| LD T,(-R16) | `ADD R16,-1` / `LD T,(R16)` |
| LD (-R16),T | `ADD R16,-1` / `LD (R16),T` |
| LD R16,(FT) | `LD l,(FT)` / `ADD FT,1` / `LD h,(FT)` / `ADD FT,-1` |
| LD (FT),R16 | `LD (FT),l` / `ADD FT,1` / `LD (FT),h` / `ADD FT,-1` |
| LD R16,(FT+) | `LD l,(FT)` / `ADD FT,1` / `LD h,(FT)` |
| LD (FT+),R16 | `LD (FT),l` / `ADD FT,1` / `LD (FT),h` |
| LD R16,(-FT) | `LD h,(FT)` / `ADD FT,-1` / `LD l,(FT)` |
| LD (-FT),R16 | `LD (FT),h` / `ADD FT,-1` / `LD (FT),l` |
| LCO T,(R16+) | `LCO T,(R16)` / `ADD R16,1` |
| LCO T,(-R16) | `ADD R16,-1` / `LCO T,(R16)` |
| EXG R8,R8 | Three `EXG` via T (neither operand is T) |
| EXG R16,R16 | Three `EXG` via FT (neither operand is FT) |
| J/CC (R16) | `J/CC' skip` / `J (R16)` / `skip:`, where CC' is the inverted condition |
| JAL addr | `LD HL,addr` / `JAL (HL)` |
| INST/CC | `J/CC' skip` / `INST` / `skip:`, where CC' is the inverted condition and `skip` is an auto-generated label |
| NOT T | `XOR T,$FF` |
| NOT FT | `XOR T,$FF` / `NOT F` |
| PICK R16,imm | `LD l,imm` / `PICK R16`, where `l` is the low byte of `R16` |
| PUSH R16/R16 | See [register lists](#register-lists) |
| POP R16/R16 | See [register lists](#register-lists) |
| SWAP R16/R16 | See [register lists](#register-lists) |

The following are native instructions, not synthesized: `LD T,(R16)`, `LD (R16),T`, `LD T,(FT)`, `LD (FT),T`, `LD R8,T`, `LD T,R8`, `LD R16,FT`, `LD FT,R16`, `EXG R8,T`, `EXG R16,FT`, `J addr`, `J/CC addr`, `J (R16)`, `JAL (R16)`, `DJ R8,addr`, `LS/RS/RSA R,imm`, `PICK R16`, `SYS imm`, `NEG R`, `NOT F`.

## Register lists

`PUSH`, `POP` and `SWAP` accept a list of 16 bit registers separated by `/`:

```
    push    bc/de/hl
    pop     ft/bc
    swap    bc/de
```

The list is order-independent and the assembler selects the compactest expansion:

| Registers | PUSH | POP | SWAP |
|---|---|---|---|
| 4 | `PUSHA` | `POPA` | `SWAPA` |
| 3 | `PUSHA` / `POP <excluded>` | `POP <excluded>` / `POPA` | `SWAPA` / `SWAP <excluded>` |
| 2 | Individual `PUSH` per register | Individual `POP` per register | Individual `SWAP` per register |

A single register is the native form (`PUSH BC`), so lists require at least two registers.

## Conditional execution

Any instruction may be made conditional by appending `/CC`:

```
    add/c   hl,1
    j/z     (bc)
```

The condition codes are `EQ`, `NE`, `LE`, `GT`, `LT`, `GE`, `LEU`, `GTU`, `LTU`, `GEU`, with the aliases `Z` (= `EQ`), `NZ` (= `NE`) and `NC` (= `GEU`). Conditional `J` with an address operand is native; conditional `J` with an indirect operand and all other instructions are synthesized as shown above.

## Warnings

Bitwise operations with a no-op immediate produce a warning: `OR $0`, `XOR $0` and `AND $FF`.

## __DCB, __RS, __DSB

|| 8 bit | 16 bit | 32 bit |
|---|---|---|---|
| Define data | DB | DW | |
| Define space | DS | | |
| Reserve symbol | RB | RW | |
