# Commodore 8 bit machine target

## Supported file formats
| Format | Option name |
|---|---|
| Binary | -fbin |
| Commodore .PRG | -fcbm |

## Pools
These machine definition declares one, two or three pools, depending on the machine configuration.

The regular `-cc64`, `-cc128`, `-c264` defines three pools. Two of these will never be used by the linker automatically, you have to specify absolute placement within these groups with the `SECTION` directive in the assembler source code file.

The main pool is the free area used for BASIC programs. Room is left for an initial `SYS` entry program. There is also a low memory pool from `$0` to `$1FF` (ZP and stack), and the high memory area above the BASIC area until `$FFFF`. It is these two last pools that will never be used by the linker automatically.

| Configuration | Start | End |
|---|---|---|
| -cc64 | $80E | 9FFF |
| -cc128 | $1C0E | EFFF |
| -cc264 | $100E | FCFF |


### Commodore 64 pools
the standard three pools, `CODE`, `DATA` and `BSS`.

There are two pools - `CHIP` and `FAST` memory. `CHIP` is 2 MiB in size, `FAST` is 1 GiB.

| Groups | Pool |
|---|---|
|`CODE`, `DATA`, `BSS` | `FAST`, `CHIP`
| `CODE_C`, `DATA_C`, `BSS_C` | `CHIP`

## Machine definitions
One predefined machine definition is available.

### Machine definition `-camiga`
| Pool | Start address | Size | Binary offset |
|------|---------------|------|---------------|
| CHIP | $00000000     | $200000 | 0 |
| FAST | $00200000     | $3FFFFFFF | $200000 |




