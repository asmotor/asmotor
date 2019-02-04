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

There is only one operator available for strings that return new strings. This is the string
concatenation operator, ```+```.

The string comparision operators (```~= == <= >= < >```) all return an integer, the result comparison operators is zero if when *false* and non-zero when *true*.