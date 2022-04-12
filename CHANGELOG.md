# 1.3.0

## Assembler
* Errors properly printed to `stderr`, noise removed
* String interpolation bugfixes
* New postfix `#` operator for forcibly inserting a string `EQUS value and concatenating
* Disallow trailing comma in DB and friends
* Symbol scope preserved with `PUSHS` and `POPS`
* Miscellaneous minor fixes

### 680x0
* ELF object file format support
* Used registers tracking implemented through `__USED_REGMASK`. New `REGMASKRESET` and `REGMASK` directives added.
* New section groups for Foenix A2560K/X

### RC8xx
* Implement latest revision and synthesized instructions
* Handle `EQUS` indirect registers

## Linker
* `-t` option marked deprecated and replaced by `-c` and `-f`
* `ELF` file format support for 680x0
* Support for Foenix A2560K/X and `PGZ` file format


# 1.2.0
## Assembler
* `ROOT` section flag added
* Unterminated `REPT`/`IF`/`MACRO` block fixes
* Error message improvements
