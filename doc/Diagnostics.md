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

Instead of the `WARN` and `FAIL` constructs, two "assert" directives are also available, which will output a warning or error message, if an expression is not equal to zero.

```
ASSERTW (\1)>=42,"Argument should be >= 42"

ASSERTW (\1)<=100,"Argument MUST be <= 100"
```
s

# Further reading
* [Introduction](Introduction.md), goals and background
* [Invoking the assembler](Assembler.md) and basic syntax
* [Symbols](Symbols.md) and labels
* [Control structures](ControlStructures.md) like ```INCLUDE```, ```MACRO```s and conditional assembling.
* [Expressions](Expressions.md) and how they're built
* [Printing diagnostic messages](Diagnostics.md), warnings and errors
* [Organising code](OrganisingCode.md) into sections. How to define data.
* [The linker](Linker.md)

# Index and reference
* [CPU specific](CpuSpecifics.md) details
* [Index of all directives](IndexDirectives.md)
* [Index of all functions](IndexFunctions.md)
* [Operator reference](ReferenceOperators.md)
* [String member reference](ReferenceStringMembers.md)
