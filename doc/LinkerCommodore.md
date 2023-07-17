# Commodore 8 bit machine target

## Supported file formats
| Format | Option name |
|---|---|
| Binary | -fbin |
| Commodore .PRG | -fcbm |

## Machine definitions
These machine definitions declares two or three pools, depending on the machine configuration.

### -cc64, -cc128, and -c264
The regular `-cc64`, `-cc128`, `-c264` defines three pools. Two of these will never be used by the linker automatically, you have to specify absolute placement within these groups with the `SECTION` directive in the assembler source code file.

The main pool is the free area used for BASIC programs. Room is reserved for an initial `SYS` entry program. There is also a low memory pool from `$0` to `$1FF` (ZP and stack), and the high memory area above the BASIC area until `$FFFF`. It is these two last pools that will never be used by the linker automatically.

| Configuration | CODE/DATA start | CODE/DATA End |
|---|---|---|
| -cc64 | $80E | 9FFF |
| -cc128 | $1C0E | EFFF |
| -cc264 | $100E | FCFF |

### -cc128f, -c128fl, and -c128fh
The configurations `-cc128f`, `-c128fl`, and `c128fh` are used to define pools suitable for creating C128 function ROMs. These will create a pool in ROM for the `CODE` and `DATA` groups, and a pool for the `BSS` group covering all RAM from $0000 to $FFFF. It is up to the user to place BSS SECTIONs at specific addresses themselves, the linker will not allocate automatically from this pool.

| Configuration | CODE/DATA start | CODE/DATA End |
|---|---|---|
| -cc128f | $8000 | $FFFF |
| -c128fl | $8000 | $BFFF |
| -c128fh | $C000 | $FFFF |
