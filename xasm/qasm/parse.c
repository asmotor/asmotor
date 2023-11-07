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


