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

| Configuration | Description | CODE/DATA size | BSS size | Shared CODE/DATA | Shared BSS |
|---|---|---|---|---|
| -chc800b | 16 KiB kernel ROM | $4000 | $4000 | $4000-$FFFF | $4000-$FFFF |
| -chc800s | Small | $10000 | $10000 | $4000-$FFFF | $4000-$FFFF |
| -chc800sh | Small Harvard |  (64 KiB text, 64 KiB data + bss)\n"
           "          -chc800m    HC800 medium mode (32 KiB text + data + bss, 32 KiB sized\n"
		   "                      banks text)\n"
           "          -chc800mh   HC800 medium Harvard executable (32 KiB text + 32 KiB\n"
		   "                      sized text banks, 64 KiB data + bss)\n"
           "          -chc800l    HC800 large mode (32 KiB text + data + bss, 32 KiB sized\n"
		   "                      banks text + data + bss)\n"


All definitions share a HOME pool from $0000 to $03FF, and a BSS pool from $C000 to $DFF7.

The unbanked definitions have a CODE/DATA pool covering the HOME pool and from $0400 to the end of the ROM image minus 16 bytes for the header.

The banked definition has a number of CODE/DATA pools, the first is from $0400 to $3FFF. The next pool is $3FF0 in size to accomodate the header, and all the last are $4000 in size and follow this in the ROM image, but they are all configured with a base address of $8000. Thus, bank #1 is unused in this configuration, the subsequent banks must be accessed though bank #2.

## Definitions

| Switch | Kind |
|---|---|
| -csms8 | Small mode 8 KiB |
| -csms16 | Small mode 16 KiB |
| -csms32 | Small mode 32 KiB |
| -csmsb | Banked mode 64+ KiB |

## Header
The header will be filled automatically by the linker.

