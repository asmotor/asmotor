# Commodore 8 bit machine target

## Supported file formats
| Format | Option name |
|---|---|
| Binary | -fbin |
| Commodore .PRG | -fcbm |

### Commodore PRG (-fcbm)

This format's entry point is the first object's first section. XLink will generate the right SYS instruction and address for you. The first section may specify a location. If not, the lowest possible BASIC address is used.

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
The Commodore 128 Function ROM is a binary format that can be burned onto an EPROM and placed internally in the C128 (or externally on a cartridge). The images must start with a particular header. XLink doesn't produce this header automatically.

The first object's first section must contain this header. Alternatively it's possible to specify a load address for a section and place it at the right address.

The configurations `-cc128f`, `-c128fl`, and `c128fh` are used to define pools suitable for creating C128 function ROMs. These will create a pool in ROM for the `CODE` and `DATA` groups, and a pool for the `BSS` group covering all RAM from $0000 to $FFFF. It is up to the user to place BSS SECTIONs at specific addresses themselves, the linker will not allocate automatically from this pool.

| Configuration | CODE/DATA start | CODE/DATA End |
|---|---|---|
| -cc128f | $8000 | $FFFF |
| -c128fl | $8000 | $BFFF |
| -c128fh | $C000 | $FFFF |
