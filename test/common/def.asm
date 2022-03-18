Sym	EQU	1

	IF	DEF(Sym)
		PRINTT "Sym exists\n"
	ELSE
		PRINTT "Sym does not exist\n"
	ENDC

	IF	~DEF(Sym)
		PRINTT "Sym does not exist\n"
	ELSE
		PRINTT "Sym exists\n"
	ENDC
