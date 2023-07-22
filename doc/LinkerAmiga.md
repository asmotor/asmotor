# Amiga machine target

## Supported file formats
| Format | Option name |
|---|---|
| Binary | -fbin |
| Hunk executable | -famigaexe |
| Hunk linkable object | -famigaobj |

### Amiga executable format (-famigaexe)

An Amiga executable's entry point is the first section. AmigaOS will simply JSR to the first address. The first object file's first section will thus be used. The order of objects therefore matters.

The Amiga format does not support placement of sections, nor does it support banks.

### Amiga link object (-famigaobj)

This output format produces an Amiga link object that can be processed by an Amiga linker. This way it's possible to use ASMotor's advanced object format and linking expressions for assembly code, and then later link with C code, for instance.

## Pools
This machine definition declares several new groups. The physical placement is only relevant when writing binary files.

There are two pools - `CHIP` and `FAST` memory. `CHIP` is 2 MiB in size, `FAST` is 1 GiB.

| Groups | Pools |
|---|---|
|`CODE`, `DATA`, `BSS` | `FAST`, `CHIP`
| `CODE_C`, `DATA_C`, `BSS_C` | `CHIP`

## Machine definitions
One predefined machine definition is available.

### Machine definition `-camiga`
| Pool | Start address | Size | Binary offset |
|------|---------------|------|---------------|
| CHIP | $00000000     | $200000 | 0 |
| FAST | $00200000     | $3FFFFFFF | $200000 |




