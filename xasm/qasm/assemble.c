#include "assemble.h"
#include "lexer.h"


static bool
assembleLine(void) {
	const SLexLine* line = lex_NextLine();

	if (line) {
		return true;
	}

	return false;
}

extern void
assemble(SLexerBuffer* buffer) {
	SLexerContext* context = lex_CreateContext(buffer);
	lex_Goto(context);

	while (assembleLine()) {
	}
}
