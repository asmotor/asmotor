# Machine Definition File
A machine definition file provides a machine definition for the linker to use when placing sections in memory. The assembler is also able to read this file, and will use it to define the groups available as `SECTION` types. This avoids having to define groups in both source files and the machine definition file.

To define a machine definition, "groups" and "pools" must be declared.

## Pools
A pool is a region in physical memory, for instance the range `$0000` to `$7FFF`. Such a pool does not need to have the same address in the CPU's address space. It may not even need to have an address in the CPU's address space, for instance when generating relocatable executable files.

A pool may also be assigned a "bank" number (discoverable using the `BANK` assembler function), and finally it may be completely omitted from a binary image.

## Groups
A group is the same as a section's type. If you have a `SECTION "Name",CODE`, then `CODE` is the name of the group in which the section will be placed. A group may consist of several pools of memory.

The same pool may be used in more than one group.

# Definition language
The language is line-based with each line defining a pool (or array of related pools), a group or the file format the machine supports.

## Comments
The language supports line based comments. Any characters after a `;` character are ignored for that line. The `;` character may appear anywhere in the line.

## Expression
A machine definition will invariably use several constants for addresses and so forth. These can also consist of some common operators and components. From highest to lowest precedence these are:

| Precedence | Component | Meaning |
|---|---|---|
| 1 | Integer | A constant number. Can be expressed in hexadecimal (prefixed by $, eg. `$1234`), binary (prefixed by `%`, eg. `%1100`), or decimal (no prefix.) |
| - | @ | An integer variable containing the array index when defining an array of pools. |
| - | symbol | The name of a symbol. |
| 2 | . | Property access. |
| 3 | + | Unary plus. |
| - | - | Unary negation. |
| - | ( ) | Parentheses for overriding precedence. |
| 4 | / | Division. |
| - | * | Multiplication. |
| 5 | + | Addition. |
| - | - | Subtraction. |

## Defining a pool

To define a single pool, the following directive is used:

```
POOL name cpu_address_expr cpu_bank_expr size_expr ?(image_offset_expr ?(:overlay))
```

| Part | Meaning |
|---|---|
| name | A name assigned to the pool, used in the machine definition file and not otherwise accessible |
| cpu_address_expr | The pool's location in the CPU's address space |
| cpu_bank_expr | The pool's bank number, an arbitrary number usually used by the CPU. May be -1 if not used |
| size_expr | The size of the pool in bytes |
| image_offset_expr | Where the pool should be placed in a binary image or overlay. If missing or -1, the pool should be omitted in the binary image, which is typically used for BSS pools. |
| overlay | The overlay file in which the pool should be placed. | 

For readability, the expression should be surrounded by parentheses, although this is not necessary.

Examples:
```
POOL bss1 $10000 0 $50000
POOL overlay2 $0 2 $E000 $0:2
```


## Array of pools
To define an array of pools, the following directive is used:

```
POOLS name[start:end] cpu_address_expr cpu_bank_expr size_expr ?(image_offset_expr ?(:overlay))
```

`name`, `cpu_address_expr`, `cpu_bank_expr`, `size_expr` and `image_offset_expr` have the same meaning as when defining a single pool. However, one or more of them will usually refer to the `@` variable in an expression, for example `(@*$8000)` when defining the pool's location in an image.

The `start` and `end` components (both inclusive) define the range, and thus number, of pools to create. The pools will be collectively referred to by `name` without any indexing.

Example:
```
POOLS banks[1:4] ($1000+@*$100) @ $100 $100 (@-1)*$100
```


## Groups

The directive to define a group is:

```
GROUP name ?(:type) pool_name_1 pool_name_2 ... pool_name_n
```

`name` is the name of the group and the section type used by assembly `SECTION` directives, this would typically be `CODE`, `BSS` and so forth. `type` is optional, and is either `TEXT` or `BSS`. If not specified, it defaults to `TEXT`

Any number of pools may follow the group's name. A pool array cannot be indexed, referring to a pool array will include all its pools.

Examples:
```
GROUP BSS:BSS bss_low bss_high data_main data_high bss1
GROUP CUBE_G data_main overlay2
```


## Symbols

Symbols can be exported by the linker and used by the assembler. The syntax is:

```
SYMBOL name expression
```

`expression`s are the same as for pools, but can all use the `.` operator. The `@` symbol is not available.

## Formats
The `FORMATS` directive is used to specify which file formats can be used with a machine definition.

```
FORMATS format_1 format_2 ... format_n
```

Any number of formats may be specified.

The available formats are:

| Id | Meaning |
|---|---|
|BIN|Binary, headerless file|
|GAME_BOY|Game Boy ROM file|
|HUNK_EXE|Amiga HUNK executable|
|HUNK_OBJ|Amiga HUNK linkable object|
|MEGA_DRIVE|SEGA Mega Drive/Genesis ROM file|
|MASTER_SYSTEM|SEGA Master System ROM file|
|HC800_KERNEL|HC800 kernel image|
|HC800|HC800 executable|
|PGZ|Foenix PGZ executable|
|KUP|Foenix F256 Kernel User Program|
|KUPP|Foenix F256 Kernel User Program, padded to 8 KiB slot size|
|COCO_QL|TRS-80 Color Computer quickload image|
|CBM_PRG[x]|Commodore .PRG at address x|
|MEGA65_PRG|MEGA65 .PRG file|

# Examples

## Mega Drive/Genesis
A machine definition for a 4 MiB Mega Drive/Genesis cartridge might look like:

```
POOL ROM $000000 -1 $400000 0
POOL RAM $FF0000 -1 $10000
GROUP CODE ROM
GROUP DATA ROM
GROUP BSS RAM
FORMATS BIN MEGA_DRIVE
```

## Game Boy
A machine definition for a cartridge with 32 banks (1024 KiB) might look like:

```
POOL HOME $0000 0 $4000 0
POOLS BANKS[1:31] $4000 @ $4000 @*$4000
POOL VRAM $8000 -1 $2000
POOL RAM $C000 -1 $2000
POOL HRAM $FF80 -1 $80
GROUP HOME HOME
GROUP CODE HOME BANKS
GROUP DATA HOME BANKS
GROUP VRAM VRAM
GROUP BSS RAM
GROUP HRAM HRAM
FORMATS BIN GAME_BOY
```
