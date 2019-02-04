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
| ```s.slice(pos,count)``` | string | ```count``` characters from ```s```, starting at ```pos``` |
| ```s.upper``` | string | Upper case version of ```s``` |
| ```s.lower``` | string | Lower case version of ```s``` |

### .slice()
The ```pos``` parameter for ```.slice()``` may also be a negative number, in which case the position is relative to the end of the string, with -1 being the last character of the string. 

```count``` may be completely omitted, in which case characters from ```pos``` until the end of the string is returned.
