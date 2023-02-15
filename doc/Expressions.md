# Expressions
ASMotor supports both integer and string expressions. Depending on the context, one or the other must be used.

## Integer expressions

Whenever the assembler expects an integer, an integer expression may be used. Integer expressions are always evaluated using signed 32 bit math.

The simplest integer expression is a number.

An expression is said to be constant when it doesn't change its value during linking. This basically means that you can't use labels in those expressions. The instructions in the macro-language all require expressions that are constant

### Integer literals
The assembler supports several numeric formats:

| Type | Example | Note |
|---|---|---|
| Hexadecimal | $0123456789ABCDEF (case-insensitive) ||
| Decimal | 0123456789 ||
| Binary | %01 ||
| Fixedpoint (16.16) | 01234.56789 ||
| Character | "ABYZ" ||
| Code | { RTS } | The value is the address of the embedded code. May span several lines |

### Operators
The assembler supports all the usual integer operators at intuitive precedence, and also a few novel ones for fixed point math.

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

### <a name="math"></a> Fixed point math
The assembler supports fixed point constants, which are normal 32 bit constants where the upper 16 bits are used for the integer part and the lower 16 bits are used for the fraction (65536ths).

Fixed point values can be used in normal integer expressions, some integer operators like plus and minus work the same whether the operands are integer or fixed point. A fixed point number can be converted to an integer by shifting it right 16 bits, as this will discard the fractional part and leave the integer part at the right bit position. It follows that you can convert an integer to a fixed point number by shifting it left 16 positions.

Other fixed point operations require more precision than 32 bit math provides, the following fixed point functions are therefore available:

| Name | Operation |
|---|---|
| x // y | x divided by y |
| x ** y | x multiplied by y |
| sin(a) | sine of the angle ```a``` |
| cos(a) | cosine of the angle ```a``` |
| tan(a) | tangent of the angle ```a``` |
| asin(a) | inverse sine of ```a``` |
| acos(a) | inverse cosine of ```a``` |
| atan(a) | inverse tangent of ```a``` |
| atan2(x,y) | inverse tangent of y/x, using the signs of both arguments to determine the quadrant of the return value |

A circle has 1.0 fixed point degrees (65536 integer), sine values are in the range [-1.0;1.0] ([-65536;65536] integer)

These functions are particularly useful for generating various tables:

```
ANGLE   SET     0.0
        REPT    256
        DB      (64.0**SIN(ANGLE)+64.0)>>16
ANGLE   SET     ANGLE+1.0/256
        ENDR
```

### <a name="symbol_functions"></a> Symbol functions

A couple of functions that deal with symbols are also available:

| Function | Meaning |
|---|---|
| bank(Symbol) | Discovers the bank number in which the symbol has been placed |
| def(Symbol) | Determines if ```Symbol``` is currently known to the assembler. Returns true if it is known, false if not |

### <a name="m68k"></a> Motorola 68K functions

The 68K assembler supports an additional function, ```regmask```, that computes the register mask of a register
list expressions:

```
Registers EQU regmask(d0-d1/d7/a6)
```

These masks are useful with a non-standard extension to the MOVEM instruction. The 68K assembler will allow you to specify a register mask instead of a register list expression like this:

```
        movem.l #Registers,(sp)+
        movem.l (sp)+,#Registers
```

The same mask can be used in both directions, the assemblers reverses it as necessary.

## String expressions

Whenever the assembler excepts a string literal, a string expression may be used instead. The simplest string expression is a string literal - a string contained in double quotes.

### <a name="string_literals"></a>String literals
A string literal is enclosed in double quotes.

```
    DC.B "This is a string",0
```

A string literal can also contain special characters, such as newlines and tabs by using escape sequences initiated by a backslash:

| Sequence | Meaning | Note |
|---|---|---
| \\\\ | \ |
| \\" | " |
| \\{ | { |
| \\} | } |
| \\n | Newline ($0A) |
| \\t | Tab ($09) |
| \\0 .. \\9 | Value of macro argument | Only valid in macros |
| \\@ | Unique label suffix | Only valid in macros and REPT blocks 

### <a name="to_string"></a> Symbol value to string operator
A string symbol is parsed as containing assembler directives and instructions. To use it as a string
as part of string expressions, it can be enclosed in pipes (```|```)

```
StringSymbol EQUS "A String"
             DB   |StringSymbol|  ; Store "A String" in memory
```

This operator can also be used with integer symbols.

### String interpolation
Within a string literal it's possible to embed the value of another expression. This is done by enclosing the expression in curly brackets:

```
StringSymbol EQUS "A String"
             DB   "Store this text and the value {|StringSymbol|.upper}",0
```

Integer expressions may also be used

```
Count EQU 8
      DB  "{Count*2}",0  ; Store the string "16"
```

The expression may otionally be followed by a comma and an alignment expression.

```
    DB  "{"Text",9}",0  ; Right align "Text" so the string "     Text" is stored.
```

If the alignment is negative, the expression is left aligned insted.

Lastly the expression may be followed by a colon and a format specifier, in the case of integer expressions.

```
    DB  "{42:X}",0  ; Store the string "2A"
```

Format specifiers support an optional precision argument:

```
    DB  "{42:X4}",0  ; Store the string "002A"
```

| Format | Meaning | Precision | Example |
|---|---|---|---|
| D | Decimal integer | The minimum number of digits printed. The number is left padded with zeroes. | {-16,6:D3} - "  -016" |
| F | Fixed point (16.16) | The exact number of decimals printed after the decimal point. | {15.6:F2} - "15.59" |
| X | Hexadecimal | The exact number of characters printed. | {$1234:X2} - "34" |
| C | Character | Ignored | {65:C} - "A" |


### String functions and properties
Several functions that work on string expressions are available. Some of these return strings and some return integers. Functions that return an integer can be used as part of integer expressions. When a string is returned the function can be used in a string expression.

Many useful functions are available, such as returning a substring, finding a substring within another string, case conversion etc. Refer to [string members reference](ReferenceStringMembers.md) for details


# Further reading
* [Introduction](Introduction.md), goals and background
* [Invoking the assembler](Assembler.md) and basic syntax
* [Symbols](Symbols.md) and labels
* [Control structures](ControlStructures.md) like ```INCLUDE```, ```MACRO```'s and conditional assembling.
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
