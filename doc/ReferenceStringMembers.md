# String member reference

## Functions returning integer
| Name | Type | Result |
|---|---|---|
| ```s.length``` | integer | The number of characters in ```s``` |
| ```s1.compareto(s2)``` | integer | Negative if ```s1``` is alphabetically < ```s2```, 0 if equal, positive if > |
| ```s1.indexof(s2)``` | integer | The position of ```s2``` within ```s1```, -1 if not found |

## Functions returning string
| Name | Type | Result |
|---|---|---|
| ```s.slice(pos,count)``` | string | ```count``` characters from ```s```, starting at ```pos```. If ```pos``` is negative, the position is relative to the end of the string, with -1 being the last character of the string. ```count``` may be completely omitted, in which case characters from ```pos``` until the end of the string is returned. |
| ```s.upper``` | string | Upper case version of ```s``` |
| ```s.lower``` | string | Lower case version of ```s``` |


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
