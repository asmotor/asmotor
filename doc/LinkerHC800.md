# HC800 machine definitions

## Supported file formats
| Format | Option name |
|---|---|
| Binary | -fbin |
| HC800 kernel | -fhc800k |
| HC800 executable | -fhc800 |

### HC800 kernel file format (-fhc800k)

This is a binary format, which is used for the HC800 kernel system software.

### HC800 executable (-fhc800)

This is an executable format which can be loaded and executed by the HC800 kernel. It includes information about the MMU and banking scheme used by the contained code.


## Pools
These machine definitions declare several pools depending on the particular configuration. The HC800 uses a separate MMU configuration when the kernel is being called (mapping in the kernel in the lower 16 KiB code and data spaces), which necessitates having an area of memory that can be accessed by both kernel and user code from $4000. This pool will vary in size depending on the particular configuration.

The HC800 system also supports a Harvard architecture with separate address spaces for CODE/DATA and BSS.

The HC800 kernel and the user program will often need to exchange data, this is done by shared memory pools.

## Groups

Several groups are used in the HC800:

| Group | Usage |
|---|---|
| HOME | CODE/DATA that must reside in the lower MMU slots for sharing between MMU configurations. |
| CODE/DATA | CODE/DATA that does not need to be places in the lower slots, but may be. May be banked. |
| BSS | Uninitialized storage area. May be banked. |
| CODE_S/DATA_S | Code/data that may be shared or used by the kernel, will never reside in the first MMU slot. |
| BSS_S | Uninitialized storage area that may be shared or used by the kernel, will never reside in the first MMU slot. |


An empty cell in the following table indicates there's no separate BSS configuration in the MMU, CODE/DATA/BSS all reside in the same RAM space.

| Configuration | Description | HOME ($0000-) size | BSS ($0000-) size | CODE_S ($4000-) size | BSS_S ($4000-) size | Bank area | Banked |
|---|---|---|---|---|---|---|---|
| -chc800b | 16 KiB kernel ROM | $4000 | $4000 | | | | |
| -chc800s | Small | $10000 | | $C000 | | | |
| -chc800sh | Small Harvard (64 KiB text, 64 KiB data + bss) | $10000 | $10000 | $C000 | $C000 | | |
| -chc800m | Medium mode (32 KiB text + data + bss, 32 KiB sized banks text) | $8000 | | $4000 | | $8000-$FFFF | CODE/DATA |
| -chc800mh | Medium Harvard (32 KiB text + 32 KiB sized text banks, 64 KiB data + bss) | $8000 | $10000 | $4000 | $4000 |  $8000-$FFFF | CODE/DATA |
| -chc800l | Large mode (32 KiB text + data + bss, 32 KiB sized banks text + data + bss) | $8000 | | $4000 | | $8000-$FFFF | CODE/DATA/BSS |
