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
| LD FT,(R16) | `LD T,(R16+)` / `LD F,(R16+)` / `ADD R16,-2` |
| LD (R16),FT | `LD (R16+),T` / `LD (R16+),F` / `ADD R16,-2` |
| LD FT,(R16+) | `LD T,(R16+)` / `LD F,(R16+)` |
| LD (R16+),FT | `LD (R16+),T` / `LD (R16+),F` |
| LD FT,(-R16) | `ADD R16,-2` / `LD T,(R16+)` / `LD F,(R16+)` / `ADD R16,-2` |
| LD (-R16),FT | `ADD R16,-2` / `LD (R16+),T` / `LD (R16+),F` / `ADD R16,-2` |
| LD T,(-R16) | `ADD R16,-1` / `LD T,(R16)` |
| LD (-R16),T | `ADD R16,-1` / `LD (R16),T` |
| LD R16,(FT) | `EXG FT,R16` / `LD T,(R16+)` / `LD F,(R16+)` / `EXG FT,R16` / `ADD FT,-2` |
| LD (FT),R16 | `EXG FT,R16` / `LD (R16+),T` / `LD (R16+),F` / `EXG FT,R16` / `ADD FT,-2` |
| LD R16,(FT+) | `EXG FT,R16` / `LD T,(R16+)` / `LD F,(R16+)` / `EXG FT,R16` |
| LD (FT+),R16 | `EXG FT,R16` / `LD (R16+),T` / `LD (R16+),F` / `EXG FT,R16` |
| LD R16,(-FT) | `ADD FT,-2` / `EXG FT,R16` / `LD T,(R16+)` / `LD F,(R16+)` / `EXG FT,R16` / `ADD FT,-2` |
| LD (-FT),R16 | `ADD FT,-2` / `EXG FT,R16` / `LD (R16+),T` / `LD (R16+),F` / `EXG FT,R16` / `ADD FT,-2` |
| LD R8,(FT+) | `LD R8,(FT)` / `ADD FT,1` (`R8` = `B`/`C`/`D`/`E`/`H`/`L`) |
| LD (FT+),R8 | `LD (FT),R8` / `ADD FT,1` (`R8` as above) |
| LD R8,(-FT) | `ADD FT,-1` / `LD R8,(FT)` (`R8` as above) |
| LD (-FT),R8 | `ADD FT,-1` / `LD (FT),R8` (`R8` as above) |
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

**Stepped access advances by the access width.** A post-increment or pre-decrement access moves its pointer by the number of bytes it touches: an 8-bit access (`LD T,(R16+)`, `LD R8,(FT+)`, …) by 1, a 16-bit access (`LD FT,(R16+)`, `LD R16,(FT+)`, …) by 2. Plain `(R16)` / `(FT)` forms do not move the pointer. The 16-bit forms above therefore step by 2, and little-endian order makes the low byte the first access. Two native steps always leave the pointer two above where they started, so every form except the post-increment walks it back with `ADD R16,-2` / `ADD FT,-2`, and the pre-decrement walks it down by 2 first. Only `CMP` writes the condition flags, so these `ADD`s and the native stepped accesses leave them untouched; the FT-as-data forms borrow the pointer through `EXG FT,R16`, and a store keeps its value in the borrowed pair across the swap.

`JAL addr` also serves as the long-jump form when a `J addr` target is outside the signed 8-bit range (−128 to +127 bytes). It has the same code size and execution time as `LD HL,addr` / `J (HL)`, but leaves HL holding the return address; use it only where HL is dead across the jump. With the usual `/CC` suffix (`JAL/NE addr`) it becomes a conditional absolute jump; HL is clobbered only when the branch is taken.

The following are native instructions, not synthesized: `LD T,(R16)`, `LD (R16),T`, `LD T,(FT)`, `LD (FT),T`, `LD R8,(FT)`, `LD (FT),R8`, `LD T,(R16+)`, `LD (R16+),T`, `LD F,(R16+)`, `LD (R16+),F` (the stepped-byte block `$11-$13` / `$15-$17` / `$19-$1B` / `$1D-$1F`: the data byte is always a hub byte — `T` or `F` — and the stepped pointer is always `BC`, `DE` or `HL`; `FT` there is illegal, the byte would land inside the pointer being stepped), `LD R8,T`, `LD T,R8`, `LD R16,FT`, `LD FT,R16`, `EXG F,T`, `EXG R16,FT`, `J addr`, `J/CC addr`, `J (R16)`, `JAL (R16)`, `DJ R8,addr`, `LS/RS/RSA R,imm`, `PICK R16,imm`, `STOW R16,imm` (all four pairs), `SYS imm`, `NEG T`, `NEG FT`, `NOT F`.

A hub byte through a *stepped FT* pointer is neither native nor synthesizable: `LD F,(FT+)` / `LD T,(FT+)` would write the half of the pointer they step, and `LD (FT+),F` / `LD (FT+),T` have no plain form to step from (`LD (FT),F` / `LD (FT),T` are not native), so all four — and the same four with `(-FT)` — are operand errors. Take the native step through a general pair instead. There is likewise no byte exchange through `T`: `EXG T,R8` is not part of the instruction set (its old opcodes now sit in the stepped-byte `LD` block), so `exg b`, `exg t,b` and `exg b,c` are operand errors. Swap bytes with `LD` through the hub, or exchange a whole pair with `EXG FT,R16`.

### Expansion cost and ranking

Every native instruction costs 4 cycles. Ranked by how many native instructions each indirect/stepped form expands to (fastest first):

| Rank | Form | Instr | Cycles | Notes |
|---|------|-------|--------|-------|
| 1 | `LD FT,(R16+)` / `LD (R16+),FT` | 2 | 8 | Two native stepped bytes — the cheapest 16-bit access |
| 2 | `LD FT,(R16)` / `LD (R16),FT` | 3 | 12 | Pair-pointer plain (walks the pointer back with `ADD R16,-2`) |
| 3 | `LD FT,(-R16)` / `LD (-R16),FT` | 4 | 16 | Pair-pointer pre-decrement (walk down, then back) |
| 3 | `LD R16,(FT+)` / `LD (FT+),R16` | 4 | 16 | FT pointer via the `EXG FT,R16` borrow dance |
| 5 | `LD R16,(FT)` / `LD (FT),R16` | 5 | 20 | Borrow dance plus pointer restore |
| 6 | `LD R16,(-FT)` / `LD (-FT),R16` | 6 | 24 | Borrow dance plus two pointer walks |
| 7 | `LIO` / `LCO` word, plain or post-inc `(R16)`/`(R16+)` | 6 | 24 | No native stepped I/O opcode; word through a pair only |
| 8 | `LIO` / `LCO` word, pre-dec `(-R16)` | 7 | 28 | |
| — | byte: `LD T,(R16)` / `LD (R16),T` / `LD R8,(FT)` / `LD (FT),R8` / `LD T,(R16+)` / `LD (R16+),T` / `LD F,(R16+)` / `LD (R16+),F` | 1 | 4 | native |
| — | byte: `LD R8,(FT+)` / `LD (FT+),R8` / `LD T,(-R16)` / `LD R8,(-FT)` … | 2 | 8 | the synthesized steppers |

Surprises worth knowing: **post-increment is cheaper than plain** for the same pointer (`LD FT,(R16+)` = 2 < `LD FT,(R16)` = 3) because the plain form must walk the pointer back — so when the pointer's end value is dead or compensable, `(R16+)` is both shorter and faster; **`BC`/`DE`/`HL` are the cheap pointers and `FT` is the expensive one** (a stepped word costs 2 through a pair, 4 through `FT`, because the native stepped-byte block steps general pairs and `FT` has to be borrowed through `EXG FT,R16`), though plain byte access through `FT` is still a single instruction; **pre-decrement is the costliest** stepping mode, since it adds a walk on top of the access; and **`LIO`/`LCO` word accesses cost more than `LD`** because there is no native stepped I/O opcode.

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
