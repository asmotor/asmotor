; TEST.Z80

FUNC:	MACRO	;funcname
	XDEF	\1
\1:
	ENDM

	FUNC	FedTest

