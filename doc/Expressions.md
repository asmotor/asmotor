# Expressions
ASMotor supports both integer and string expressions. Depending on the context, one or the other must be used.

## Integer expressions

Whenever the assembler expects an integer, an integer expression may be used. Integer expressions are always evaluated using signed 32 bit math.

The simplest integer expression is a number.

An expression is said to be constant when it doesn't change its value during linking. This basically means that you can't use labels in those expressions. The instructions in the macro-language all require expressions that are constant

### Integer literals
The assembler supports several numeric formats:

| Type | Example |
|---|---|
| Hexadecimal | $0123456789ABCDEF (case-insensitive) |
| Decimal | 0123456789 |
| Binary | %01 |
| Fixedpoint (16.16) | 01234.56789 |
| Character | "ABYZ" |

### Operators
Several operators can be used to build up integer expressions. In order of precedence they are:

| Operator | Meaning |
|---|---|
| ( ) | Precedence override |
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

### Fixed point math
The assembler supports fixed point constants, which are normal 32 bit constants where the upper 16 bits are used for the integer part and the lower 16 bits are used for the fraction (65536ths).

Fixed point values can be used in normal integer expressions, some integer operators like plus and minus work the same whether the operands are integer or fixed point. A fixed point number can be converted to an integer by shifting it right 16 bits, as this will discard the fractional part and leave the integer part at the right bit position. It follows that you can convert an integer to a fixed point number by shifting it left 16 positions.

Other fixed point operations require more precision than 32 bit math provides, the following fixed point functions are therefore available:

| Name | Operation |
|---|---|
| x // y | x divided by y |
| x ** y | x multiplied by y |
| SIN(x) | sin(x) |
| COS(x) | cos(x) |
| TAN(x) | tan(x) |
| ASIN(x) | asin(x) |
| ACOS(x) | acos(x) |
| ATAN(x) | atan(x) |
| ATAN2(x,y) | Angle of the vector [x,y] |

A circle has 1.0 fixed point degrees (65536 integer), sine values are in the range [-1.0;1.0]

These functions are particularly useful for generating various tables:

```
ANGLE   SET     0.0
        REPT    256
        DB      (64.0**SIN(ANGLE)+64.0)>>16
ANGLE   SET     ANGLE+1.0/256
        ENDR
```

## String expressions

Whenever the assembler excepts a string literal, a string expression may be used instead. The simplest string expression is a string literal - a string contained in double quotes.

### String literals
A string literal is enclosed in double quotes.

```
    DC.B "This is a string",0
```

A string literal can also contain special characters, such as newlines and tabs by using escape sequences initiated by a backslash:

| Sequence | Meaning | Note |
|---|---|---
| \\ | \ |
| \\" | " |
| \\{ | { |
| \\} | } |
| \\n | Newline ($0A) |
| \\t | Tab ($09) |
| \\0 .. \\9 | Value of macro argument | Only valid in macros |
| \\@ | Unique label suffix | Only valid in macros and REPT blocks 

### String interpolation
Within a string literal it's possible to embed the value of a symbol. This is done by enclosing the symbol name in curly brackets:

```
StringSymbol EQUS "A String"
             DB   "Store this text and the value {StringSymbol}",0
```

As a shorthand, a symbol can simply be surrounded by curly brackets outside a string literal, to convert its value to a string expression:

```
StringSymbol EQUS "Another string"
             DB   {StringSymbol}
````

### String functions and properties
Several functions that work on string expressions are available. Some of these return strings and some return integers. Functions that return an integer can be used as part of integer expressions, when a string is returned the function can be used in a string expression.

| Name | Type | Result |
|---|---|---|
| ```s.length``` | integer | The number of characters in s |
| ```s1.compareto(s2)``` | integer | Negative if s1 is alphabetically < | s2, 0 if equal, positive if > |
| ```s1.indexof(s2)``` | integer | The position of s2 within s1, -1 if not found |
| ```s1==s2``` | integer | Non-zero if s1 is equal to s2, otherwise zero |
| ```~= < <= > >=``` | integer | String comparison operators. |
| ```s1+s2```	| string | String concatenation, s1 followed by s2 |
| ```s.slice(pos,count)``` | string | count characters from s, starting at pos |
| ```s.toupper()``` | string | Upper case version of s |
| ```s.tolower()``` | string | Lower case version of s |

The pos parameter for ```.slice()``` may also be a negative number, in which case the position is relative to the end of the string, with -1 being the last character of the string. count may be completely omitted, in which case characters from pos until the end of the string is returned.
