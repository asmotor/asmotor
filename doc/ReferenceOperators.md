# Operator reference

## <a name="integer_operations"></a> Integer and fixed point operators

Several operators can be used to build up integer expressions. In order of precedence they are:

| Operator | Meaning |
|---|---|
| ( ) | Precedence override |
| string comparison (~= == <= >= < >), string member function | String operations returning int |
| FunctionName(...) | Function call |
| ~ + - | Unary bitwise not, plus, negation |
| * / ~/ ** // | Multiply, divide, modulo, fixedpoint multiply and divide |
| << >> | Shift left, shift right |
| & ^ \| |	Bitwise and, bitwise exclusive or, bitwise or |
| + - | Add, subtract |
| ~= == <= >= < > | Comparison operators: not equal, equal, less than or equal, greater than or equal, less than, greater than |
| && || | Boolean and, boolean or |
| ~! | Unary boolean not |

The result of the boolean operators and comparison operators is zero if when *false* and non-zero when *true*.

## String operators

There are only a couple of operators available that return new strings. One is the string
concatenation operator, ```+```. The other is the ["to string"](Expressions.md#to_string) operator - ```|expression|```.

The string comparision operators (```~= == <= >= < >```) all return an integer, the result comparison operators is zero if when *false* and non-zero when *true*.


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
