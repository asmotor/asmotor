SOFFSET SET 4

LONG    MACRO
\1      EQU     SOFFSET
SOFFSET SET     SOFFSET+4
        ENDM

        LONG    MyLong  * Comment, more comments

        PRINTT  "MyLong = {|MyLong|}\n"
        PRINTT  "SOFFSET = {|SOFFSET|}\n"

ID      EQU     ('B'<<24)!('A'<<16)!('D'<<8)

        PRINTV  ID


        SECTION "Test",CODE

