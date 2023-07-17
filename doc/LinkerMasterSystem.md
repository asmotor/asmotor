# Sega Mega Drive/Genesis machine definition

## Supported file formats
| Format | Option name |
|---|---|
| Binary | -fbin |
| Sega Master System | -fsms |

## Pools
This machine definition declares several pools and groups. There's a few unbanked definitions, and a banked definition for 64+ KiB images.

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

