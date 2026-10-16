# Commodore 8 bit machine target

## Supported file formats
| Format | Option name |
|---|---|
| Binary | -f bin |
| Commodore .PRG | -f cbm |
| MEGA65 .PRG | -f mega65 |

### Commodore PRG (-f cbm)

This format's entry point is the first object's first section. XLink will generate the right `SYS` BASIC command and address for you. The first section may specify a location. If not, the lowest possible BASIC address is used.

If the linker's `-s` or `-e` options are used, these specify the entry address for the `SYS` instruction.

## Machine definitions
These machine definitions declares two or three pools, depending on the machine configuration.

### -c c64, -c c128, -c 264, and -c mega65
The regular `-c c64`, `-c c128`, `-c 264`, and `-c mega65` define three pools. Two of these will never be used by the linker automatically, you have to specify absolute placement within these groups with the `SECTION` directive in the assembler source code file.

The main pool is the free area used for BASIC programs. Room is reserved for an initial `SYS` entry program. There is also a low memory pool from `$0` to `$1FF` (ZP and stack), and the high memory area above the BASIC area until `$FFFF`. It is these two last pools that will never be used by the linker automatically.

| Configuration | CODE/DATA start | CODE/DATA End |
|---|---|---|
| -c c64 | $80E | $9FFF |
| -c c128 | $1C0E | $EFFF |
| -c c264 | $100E | $FCFF |
| -c mega65 | $2012 | $F6FF |

### -c c128f, -c 128fl, and -c 128fh
The Commodore 128 Function ROM is a binary format that can be burned onto an EPROM and placed internally in the C128 (or externally on a cartridge). The images must start with a particular header. XLink doesn't produce this header automatically.

The first object's first section must contain this header. Alternatively it's possible to specify a load address for a section and place it at the right address.

The configurations `-c c128f`, `-c 128fl`, and `c128fh` are used to define pools suitable for creating C128 function ROMs. These will create a pool in ROM for the `CODE` and `DATA` groups, and a pool for the `BSS` group covering all RAM from $0000 to $FFFF. It is up to the user to place BSS SECTIONs at specific addresses themselves, the linker will not allocate automatically from this pool.

| Configuration | CODE/DATA start | CODE/DATA End |
|---|---|---|
| -c c128f | $8000 | $FFFF |
| -c 128fl | $8000 | $BFFF |
| -c 128fh | $C000 | $FFFF |
