# Diagnostic messages
While assembling it is possible to cause the assembler to print out various user defined messages.

Simple diagnostic messages are possible with ```PRINTT```, ```PRINTF``` and ```PRINTV```.

```
PRINTT "A simple message\n" ; Remember \n for newline
PRINTV (2+3)/5              ; Prints an integer
PRINTF 3.14**2              ; Prints a fixed point value
```

See also ["string literals"](Expressions.md#string_literals) for more advanced formatting options.

In macros it can be helpful to warn the user of a wrong argument or completely abort the assembly process. This is possible with the ```FAIL``` and ```WARN``` commands. ```FAIL``` and ```WARN``` take a string as the only argument and will print it out as a regular warning or error with a line number.

```FAIL``` stops assembling immediately while WARN continues after printing the error message.

```
IF (\1)<42
WARN "Argument should be >= 42"
ENDC

IF (\1)>100
FAIL "Argument MUST be <= 100"
ENDC
```


# Further reading
* [Introduction](doc/Introduction.md), goals and background
* [Invoking the assembler](doc/Assembler.md) and basic syntax
* [Symbols](doc/Symbols.md) and labels
* [Control structures](doc/ControlStructures.md) like ```INCLUDE```, ```MACRO```'s and conditional assembling.
* [Expressions](doc/Expressions.md) and how they're built
* [Printing diagnostic messages](doc/Diagnostics.md), warnings and errors
* [Organising code](doc/OrganisingCode.md) into sections. How to define data.
* [The linker](doc/Linker.md)

# Index and reference
* [CPU specific](doc/CpuSpecifics.md) details
* [Index of all directives](doc/IndexDirectives.md)
* [Index of all functions](doc/IndexFunctions.md)
* [Operator reference](doc/ReferenceOperators.md)
* [String member reference](doc/ReferenceStringMembers.md)
