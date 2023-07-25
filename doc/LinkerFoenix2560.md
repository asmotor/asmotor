# Foenix A2560K/X machine definitions

## Supported file formats
| Format | Option name |
|---|---|
| Binary | -fbin |
| Foenix .PGZ | -ffxpgz |

### Foenix executable (-ffxpgz)

This is an executable format which can be loaded and executed by the FoenixMCP kernel.

## Pools
This machine definition declares several pools covering different areas of the address space. The IO hole at $C000-$DFFF is excluded in this configuration. More advanced configurations can be defined by the user by providing a linker machine definition file.

| Pool | Area | Description |
|---|---|---|
| ZERO_PAGE | $10-$FF | Zero page. |
| MAIN_RAM | $200-$BFFF | Main memory area. |
| HIGH_RAM | $E000-$FFFF | High memory area. |

The HIGH_RAM pool covers vectors and is useful for code common to all MMU configurations.

## Groups

| Group | Pools |
|---|---|
| HOME | HIGH_RAM |
| CODE | MAIN_RAM, HIGH_RAM |
| DATA | MAIN_RAM |
| BSS | MAIN_RAM |
| ZP| ZERO_PAGE |