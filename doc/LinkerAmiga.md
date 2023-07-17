# Amiga machine target

## Supported file formats
| Format | Option name |
|---|---|
| Binary | -fbin |
| Hunk executable | -famigaexe |
| Hunk linkable object | -famigaobj |

## Pools
This machine definition declares several new groups. The physical placement is only relevant when writing binary files.

There are two pools - `CHIP` and `FAST` memory. `CHIP` is 2 MiB in size, `FAST` is 1 GiB.

| Groups | Pool |
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




