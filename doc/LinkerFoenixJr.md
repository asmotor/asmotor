# Foenix A2560K/X machine definitions

## Supported file formats
| Format | Option name |
|---|---|
| Binary | -fbin |
| Foenix .PGZ | -ffxpgz |

### Foenix executable (-ffxpgz)

This is an executable format which can be loaded and executed by the FoenixMCP kernel.

## Pools
This machine definition declares several pools covering different areas of the address space.

| Pool | Area | Description |
|---|---|---|
| SYSRAM | $10000-$3FFFFF | Main system memory after the kernel |
| VICKY_A | $800000-$BFFFFF | Memory accessible by VICKY A |
| VICKY_B | $C00000-$FFFFFF | Memory accessible by VICKY B |
| SDRAM | $2000000-$5FFFFFF | SDRAM |

## Groups

| Group | Pools |
|---|---|
| CODE | SYSRAM |
| DATA | SYSRAM, SDRAM |
| BSS | SYSRAM, SDRAM |
| DATA_VA, BSS_VA | VICKY_A |
| DATA_VB, BSS_VB | VICKY_B |
| DATA_D, BSS_D | SDRAM |
