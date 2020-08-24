	SECTION	"Code",CODE[0]

	__DCL	Stack

	SECTION	"Vars",BSS

	__DSB	256
Stack:

	SECTION	"MoreCode",CODE

Here:
	__DCW	Here
	__DCW	Stack2
	__DCW	Stack3

	SECTION	"MoreVars",BSS

Stack2:
	__DSB	256
Stack3:

