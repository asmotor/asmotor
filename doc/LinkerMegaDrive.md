# Sega Mega Drive/Genesis machine definition

## Supported file formats
| Format | Option name |
|---|---|
| Binary | -fbin |
| Sega Mega Drive/Genesis ROM | -fsmd |

### Sega Mega Drive/Genesis file format (-smd)

This is a binary format. A Mega Drive image must start with a particular header, the linker does not produce this header automatically, but will update it.

The first object's first section must contain this header, alternatively it's possible to specify a load address for a section and place it at the right address. The linker will update the header's ROM ending address and checksum bytes accordingly.

## Pools
This machine definition declares several pools and groups, with an almost one-to-one relationship.

| Pool/group | Start | End |
|---|---|---|
| CODE/DATA | $000000 | $3FFFFF | 
| BSS | $FF0000 | $FFFFFF |

## Header
The header must be filled correctly by the user, but the linker will correct the length entry at $1A4, and the checksum at $18E.
