# RC800

The RC800 is the CPU of the HC800 computer. This document covers assembler-specific behavior.

## Section alignment
Sections are not aligned to any particular multiple.

## Endianness
Multi-byte values are emitted least significant byte first by default.

## Command line

### -m s\<x> option
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
| ADD FT,FT (or `ADD FT`) | `LS FT,1` |
| SUB R,imm | `ADD R,-imm` |
| LD R16,imm | Two 8 bit loads, with a 3 byte optimization: `EXT` if the value is sign-extendable, `LD F,T` if both bytes are equal |
| LD FT,(R16) | `EXG FT,R16` / `LD R16.lo,(FT+)` / `LD R16.hi,(FT)` / `ADD FT,-1` / `EXG FT,R16` |
| LD (R16),FT | `EXG FT,R16` / `LD (FT+),R16.lo` / `LD (FT),R16.hi` / `ADD FT,-1` / `EXG FT,R16` |
| LD FT,(R16+) | `EXG FT,R16` / `LD R16.lo,(FT+)` / `LD R16.hi,(FT+)` / `EXG FT,R16` |
| LD (R16+),FT | `EXG FT,R16` / `LD (FT+),R16.lo` / `LD (FT+),R16.hi` / `EXG FT,R16` |
| LD FT,(-R16) | `ADD R16,-2` / `EXG FT,R16` / `LD R16.lo,(FT+)` / `LD R16.hi,(FT)` / `ADD FT,-1` / `EXG FT,R16` |
| LD (-R16),FT | `ADD R16,-2` / `EXG FT,R16` / `LD (FT+),R16.lo` / `LD (FT),R16.hi` / `ADD FT,-1` / `EXG FT,R16` |
| LD T,(R16+) | `LD T,(R16)` / `ADD R16,1` |
| LD (R16+),T | `LD (R16),T` / `ADD R16,1` |
| LD T,(-R16) | `ADD R16,-1` / `LD T,(R16)` |
| LD (-R16),T | `ADD R16,-1` / `LD (R16),T` |
| LD R16,(FT) | `LD R16.lo,(FT+)` / `LD R16.hi,(FT)` / `ADD FT,-1` |
| LD (FT),R16 | `LD (FT+),R16.lo` / `LD (FT),R16.hi` / `ADD FT,-1` |
| LD R16,(FT+) | `LD R16.lo,(FT+)` / `LD R16.hi,(FT+)` |
| LD (FT+),R16 | `LD (FT+),R16.lo` / `LD (FT+),R16.hi` |
| LD R16,(-FT) | `ADD FT,-2` / `LD R16.lo,(FT+)` / `LD R16.hi,(FT)` / `ADD FT,-1` |
| LD (-FT),R16 | `ADD FT,-2` / `LD (FT+),R16.lo` / `LD (FT),R16.hi` / `ADD FT,-1` |
| LIO FT,(R16) | `LIO T,(R16)` / `ADD R16,1` / `EXG F,T` / `LIO T,(R16)` / `ADD R16,-1` / `EXG F,T` |
| LIO (R16),FT | `LIO (R16),T` / `ADD R16,1` / `EXG F,T` / `LIO (R16),T` / `ADD R16,-1` / `EXG F,T` |
| LIO FT,(R16+) | `LIO T,(R16)` / `ADD R16,1` / `EXG F,T` / `LIO T,(R16)` / `ADD R16,1` / `EXG F,T` |
| LIO (R16+),FT | `LIO (R16),T` / `ADD R16,1` / `EXG F,T` / `LIO (R16),T` / `ADD R16,1` / `EXG F,T` |
| LIO FT,(-R16) | `ADD R16,-2` / `LIO T,(R16)` / `ADD R16,1` / `EXG F,T` / `LIO T,(R16)` / `ADD R16,-1` / `EXG F,T` |
| LIO (-R16),FT | `ADD R16,-2` / `LIO (R16),T` / `ADD R16,1` / `EXG F,T` / `LIO (R16),T` / `ADD R16,-1` / `EXG F,T` |
| LCO FT,(R16) | `LCO T,(R16)` / `ADD R16,1` / `EXG F,T` / `LCO T,(R16)` / `ADD R16,-1` / `EXG F,T` |
| LCO FT,(R16+) | `LCO T,(R16)` / `ADD R16,1` / `EXG F,T` / `LCO T,(R16)` / `ADD R16,1` / `EXG F,T` |
| LCO FT,(-R16) | `ADD R16,-2` / `LCO T,(R16)` / `ADD R16,1` / `EXG F,T` / `LCO T,(R16)` / `ADD R16,-1` / `EXG F,T` |
| LCO T,(R16+) | `LCO T,(R16)` / `ADD R16,1` |
| LCO T,(-R16) | `ADD R16,-1` / `LCO T,(R16)` |
| EXG R16,R16 | Three `EXG` via FT (neither operand is FT) |
| ADD/SUB/AND/OR/XOR R8,R8 (dest = F) | `EXG F,T` / `OP T,R8` / `EXG F,T` (see [register-to-register ALU](#register-to-register-alu)) |
| ADD/SUB R16,R16 (dest ≠ FT) | `EXG R16,FT` / `OP FT,R16` / `EXG R16,FT` (see [register-to-register ALU](#register-to-register-alu)) |
| CMP R8,R8 (dest = F) | `EXG F,T` / `CMP T,R8` (see [register-to-register ALU](#register-to-register-alu)) |
| CMP R16,R16 (dest ≠ FT) | `PUSH R16` / `EXG FT,R16` / `CMP FT,R16` / `LD T,R16.lo` / `POP R16` (see [register-to-register ALU](#register-to-register-alu)) |
| NEG F | `NOT F` / `ADD F,$01` |
| NEG R16 (non-hub register) | `EXG R16,FT` / `NEG FT` / `EXG R16,FT` |
| J/CC (R16) | `J/CC' skip` / `J (R16)` / `skip:`, where CC' is the inverted condition |
| JAL addr | `LD HL,addr` / `JAL (HL)` |
| INST/CC | `J/CC' skip` / `INST` / `skip:`, where CC' is the inverted condition and `skip` is an auto-generated label |
| NOT T | `XOR T,$FF` |
| NOT FT | `XOR T,$FF` / `NOT F` |
| PUSH R16/R16 | See [register lists](#register-lists) |
| POP R16/R16 | See [register lists](#register-lists) |
| SWAP R16/R16 | See [register lists](#register-lists) |

**Stepped access advances by the access width.** A post-increment or pre-decrement access moves its pointer by the number of bytes it touches: an 8-bit access (`LD T,(R16+)`, `LD R8,(FT+)`, …) by 1, a 16-bit access (`LD FT,(R16+)`, `LD R16,(FT+)`, …) by 2. Plain `(R16)` / `(FT)` forms do not move the pointer. The 16-bit forms above therefore step by 2, and `R16.lo` / `R16.hi` are the pointer pair's own byte halves (little-endian, low byte first). Because `LD R8,(FT+)` / `LD (FT+),R8` and `EXG FT,R16` are native, these expansions use no ALU op and leave the condition flags untouched (only `CMP` writes them); the FT-as-data forms borrow the pointer through `EXG FT,R16`, and a store preserves its FT value across the swap.

`JAL addr` also serves as the long-jump form when a `J addr` target is outside the signed 8-bit range (−128 to +127 bytes). It has the same code size and execution time as `LD HL,addr` / `J (HL)`, but leaves HL holding the return address; use it only where HL is dead across the jump. With the usual `/CC` suffix (`JAL/NE addr`) it becomes a conditional absolute jump; HL is clobbered only when the branch is taken.

The following are native instructions, not synthesized: `LD T,(R16)`, `LD (R16),T`, `LD T,(FT)`, `LD (FT),T`, `LD R8,(FT+)`, `LD (FT+),R8` (the stepped-byte block `$1A-$1F` / `$12-$17`, `R8` = `B`/`C`/`D`/`E`/`H`/`L`), `LD R8,T`, `LD T,R8`, `LD R16,FT`, `LD FT,R16`, `EXG F,T`, `EXG R16,FT`, `J addr`, `J/CC addr`, `J (R16)`, `JAL (R16)`, `DJ R8,addr`, `LS/RS/RSA R,imm`, `PICK R16,imm`, `STOW R16,imm` (all four pairs), `SYS imm`, `NEG T`, `NEG FT`, `NOT F`.

There is no byte exchange through `T`: `EXG T,R8` is not part of the instruction set (its old opcodes `$12-$17` now form the stepped-byte `LD (FT+),R8` block), so `exg b`, `exg t,b` and `exg b,c` are operand errors. Swap bytes with `LD` through the hub, or exchange a whole pair with `EXG FT,R16`.

### Expansion cost and ranking

Every native instruction costs 4 cycles. Ranked by how many native instructions each indirect/stepped form expands to (fastest first):

| Rank | Form | Instr | Cycles | Notes |
|---|------|-------|--------|-------|
| 1 | `LD R16,(FT+)` / `LD (FT+),R16` | 2 | 8 | Two native stepped bytes — the cheapest 16-bit access |
| 2 | `LD R16,(FT)` / `LD (FT),R16` | 3 | 12 | FT-pointer plain (adds `ADD FT,-1` to restore the pointer) |
| 3 | `LD FT,(R16+)` / `LD (R16+),FT` | 4 | 16 | Pair-pointer post-increment |
| 3 | `LD R16,(-FT)` / `LD (-FT),R16` | 4 | 16 | FT-pointer pre-decrement |
| 5 | `LD FT,(R16)` / `LD (R16),FT` | 5 | 20 | Pair-pointer plain |
| 6 | `LD FT,(-R16)` / `LD (-R16),FT` | 6 | 24 | Pair-pointer pre-decrement |
| 7 | `LIO` / `LCO` word, plain or post-inc `(R16)`/`(R16+)` | 6 | 24 | No native stepped I/O opcode; word through a pair only |
| 8 | `LIO` / `LCO` word, pre-dec `(-R16)` | 7 | 28 | |
| — | byte: `LD T,(R16)` / `LD R8,(FT)` / `LD R8,(FT+)` … | 1 | 4 | `LD R8,(FT+)` / `LD (FT+),R8` are native stepped bytes |
| — | byte: `LD T,(R16+)` / `LD R8,(-FT)` … | 2 | 8 | |

Surprises worth knowing: **post-increment is cheaper than plain** for the same pointer (`LD FT,(R16+)` = 4 < `LD FT,(R16)` = 5; `LD R16,(FT+)` = 2 < `LD R16,(FT)` = 3) because the plain form must walk the borrowed pointer back with `ADD FT,-1` — so when the pointer's end value is dead or compensable, `(R16+)` is both shorter and faster; **a word through `FT` (2–3) is far cheaper than through `BC`/`DE`/`HL` (4–6)**, since FT has native stepped-byte opcodes while a general pair is borrowed via `EXG FT,R16`; **pre-decrement is the costliest** stepping mode; and **`LIO`/`LCO` word accesses cost more than `LD`** because there is no native stepped I/O opcode.

### Register-to-register ALU

All data flows through the T/FT hub, so `ADD`, `SUB`, `AND`, `OR`, `XOR` and `CMP` natively take only `T` (8 bit) or `FT` (16 bit) as the destination. The 16 bit register-to-register combinations are synthesized as an exchange thunk; the 8 bit ones only when the destination is `F`, whose exchange is the native `EXG F,T`:

```
    sub bc,de   →   exg bc,ft / sub ft,de / exg bc,ft
    add f,d     →   exg f,t   / add t,d   / exg f,t
    cmp f,c     →   exg f,t   / cmp t,c
```

The exchanges restore both operands and the hub register afterwards; nothing besides the intended result is modified. When the source operand is the hub register itself (`add f,t`), the middle instruction reads the destination instead, which holds the pre-exchange hub value.

A byte destination other than `F` has no exchange to open with — the byte exchange through `T` (`EXG T,R8`) is not part of the instruction set — so `add b,d`, `cmp e,h`, `neg c` and friends are operand errors whether or not synthesis is enabled. Spell them out:

```
    add c,d     →   exg bc,ft / add t,d / exg bc,ft         (3, hub value restored)
    add b,d     →   ld t,b / add t,d / ld b,t               (3 if T is free, else 5)
```

The pair exchange is the cheap route for a pair's low byte: `EXG FT,BC` swaps `T`↔`C` *and* `F`↔`B` at once, so the low byte reaches the hub in one instruction and the hub's own value comes back untouched. A high byte needs two more instructions to get from `F` to `T` and back.

Notes:

- `CMP` expands to `EXG F,T` / `CMP T,R8` for a byte destination: the destination is the flags register itself, so there is no closing exchange — the fresh flags stay in F and `T` holds the old F value. For 16 bit destinations it expands to the five-instruction push/exchange form above (20 cycles, one transient pair-stack slot, balanced even under `/CC`); when FT is free, a native `CMP FT,R16` after `LD FT,R16` is cheaper.
- `AND`, `OR` and `XOR` are 8 bit instructions, so they have no 16 bit forms.
- Width-mismatched operands (`add b,de`) remain an error.
- Self operations (`OP X,X`) use a direct expansion instead of the thunk, except byte doubling (`add b,b`), which would need the byte exchange and stays an error:

| Form | Expansion |
|---|---|
| `ADD F,F` | double via the hub: `EXG F,T` / `ADD T,T` / `EXG F,T` |
| `ADD R16,R16` | `EXG R16,FT` / `LS FT,1` / `EXG R16,FT` |
| `SUB X,X`, `XOR X,X` | `LD X,0` (16 bit via the immediate-load synthesis) |
| `AND X,X`, `OR X,X` | warning, emits nothing |
| `CMP X,X` | warning, emits `LD F,$02` (FLAGS_EQ — the comparison is statically equal) |

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

The condition codes are `EQ`, `NE`, `LE`, `GT`, `LT`, `GE`, `LEU`, `GTU`, `LTU`, `GEU`, with the aliases `Z` (= `EQ`), `NZ` (= `NE`), `CC` (= `GEU`, carry clear) and `CS` (= `LTU`, carry set). Conditional `J` with an address operand is native; conditional `J` with an indirect operand and all other instructions are synthesized as shown above.

## Warnings

Bitwise operations with a no-op immediate produce a warning: `OR $0`, `XOR $0` and `AND $FF`.

Self operations that collapse produce a warning: `AND X,X` and `OR X,X` (no-op, emits nothing) and `CMP X,X` (always equal, emits `LD F,$02`).

## __DCB, __RS, __DSB

|| 8 bit | 16 bit | 32 bit |
|---|---|---|---|
| Define data | DB | DW | |
| Define space | DS | | |
| Reserve symbol | RB | RW | |
