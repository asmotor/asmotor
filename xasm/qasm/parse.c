#include "lexer.h"
#include "parse.h"


extern SExpression*
parse_Expression(size_t maxStringConstLength) {
	internalerror("parse_Expression not implemented");
}


extern bool
parse_ExpectChar(char ch) {
	internalerror("parse_ExpectChar not implemented");
}


extern int64_t
parse_ConstantExpression(size_t maxStringConstLength) {
	internalerror("parse_ConstantExpression not implemented");
}


extern void
parse_GetToken(void) {
	lex_NextToken();
}


