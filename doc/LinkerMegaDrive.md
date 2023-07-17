# Sega Mega Drive/Genesis machine definition

## Supported file formats
| Format | Option name |
|---|---|
| Binary | -fbin |
| Sega Mega Drive/Genesis ROM | -fsmd |

## Pools
This machine definition declares several pools and groups, with an almost one-to-one relationship.

| Pool/group | Start | End |
|---|---|---|
| CODE/DATA | $000000 | $3FFFFF | 
| BSS | $FF0000 | $FFFFFF |

## Header
The header must be filled correctly by the user, but the linker will correct the length entry at $1A4, and the checksum at $18E.
