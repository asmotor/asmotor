#include "lexer.h"
#include "parse.h"


extern bool
parse_ExpectChar(char ch) {
	internalerror("parse_ExpectChar not implemented");
}


extern void
parse_GetToken(void) {
	lex_NextToken();
}


extern SSymbol*
parse_Symbol(void) {
	if (lex_Context->token.id == T_ID) {
		string* name = lex_TokenString();
		SSymbol* symbol = sym_Find(name);
		str_Free(name);

		return symbol;
	}
	return NULL;
}

