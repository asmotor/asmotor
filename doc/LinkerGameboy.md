# Nintendo Game Boy machine definitions

## Supported file formats
| Format | Option name |
|---|---|
| Binary | -fbin |
| Nintendo Game Boy ROM | -fngb |

## Machine definitions
These machine definitions declare either an unbanked 32 KiB ROM image or a banked ROM image which can be much larger.

They share some common pools and groups:
| Pool/group | Start | End |
|---|---|---|
| VRAM | $8000 | $9FFF | 
| BSS | $C000 | $DFFF |
| HRAM | $FF80 | $FFFF |

### -cngbs
This machine definition is used for small mode 32 KiB images. It uses the common pools, and pool defined for the ROM area $0000 to $7FFF. The groups HOME, CODE and DATA all refer to this pool.

### -cngb
This machine definition is used for images larger than 32 KiB. It is banked with each bank being 16 KiB in size. It uses the common pools, one pool for the HOME group, and several pools for CODE/DATA groups.

The HOME group is the area from $0000 to $3FFF, and the CODE/DATA group is several banks mapped into the $4000 to $7FFF. The linker will place CODE/DATA sections in these pools, starting from the pool at the beginning of the ROM image. The bank number can be discovered with the BANK assembler function, and can be used to map the BANK into memory from code.

## Header
THe header must be more or less filled correctly by the user, but the linker will correct the following entries:

| Entry | Location |
|---|---|
| "Magic bytes" | $000 |
| Cartridge type | $147 |
| Cartridge size | $148 |
| Checksum | $14D, $14E |
