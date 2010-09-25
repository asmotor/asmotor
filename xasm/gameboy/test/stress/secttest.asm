
ExportEQU	EQU	4
	EXPORT	ExportEQU

	SECTION	"Code",HOME

	ld	hl,Stack

	SECTION	"Vars",BSS

	DS	256
Stack: