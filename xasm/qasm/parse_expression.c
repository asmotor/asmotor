#include "lexer.h"
#include "parse.h"
#include "parse_expression.h"


#define FIRST_PRECEDENCE_TOKEN T_OP_EQUAL
#define TOTAL_PRECEDENCES (T_OP_MODULO - FIRST_PRECEDENCE_TOKEN + 1)

#define PRECEDENCE_COMPARE 0
#define PRECEDENCE_BOOLEAN_LOGIC 1
#define PRECEDENCE_BITWISE_LOGIC 2
#define PRECEDENCE_BITWISE_MUL 3
#define PRECEDENCE_ADD 4
#define PRECEDENCE_UNARY 5		// Not necessary, but here for documentation

static int s_precedence[] = {
	PRECEDENCE_COMPARE,	// T_OP_EQUAL
	PRECEDENCE_COMPARE,	// T_OP_NOT_EQUAL
	PRECEDENCE_COMPARE,	// T_OP_GREATER_THAN
	PRECEDENCE_COMPARE,	// T_OP_GREATER_OR_EQUAL
	PRECEDENCE_COMPARE,	// T_OP_LESS_THAN
	PRECEDENCE_COMPARE,	// T_OP_LESS_OR_EQUAL

	PRECEDENCE_BOOLEAN_LOGIC,	// T_OP_BOOLEAN_OR
	PRECEDENCE_BOOLEAN_LOGIC,	// T_OP_BOOLEAN_AND

	PRECEDENCE_UNARY,	// T_OP_BOOLEAN_NOT

	PRECEDENCE_BITWISE_LOGIC,	// T_OP_BITWISE_OR
	PRECEDENCE_BITWISE_LOGIC,	// T_OP_BITWISE_XOR
	PRECEDENCE_BITWISE_LOGIC,	// T_OP_BITWISE_AND

	PRECEDENCE_BITWISE_MUL,	// T_OP_BITWISE_ASL
	PRECEDENCE_BITWISE_MUL,	// T_OP_BITWISE_ASR

	PRECEDENCE_UNARY,	// T_OP_BITWISE_NOT

	PRECEDENCE_ADD,	// T_OP_ADD
	PRECEDENCE_ADD,	// T_OP_SUBTRACT

	PRECEDENCE_BITWISE_MUL,	// T_OP_MULTIPLY
	PRECEDENCE_BITWISE_MUL,	// T_OP_DIVIDE
	PRECEDENCE_BITWISE_MUL,	// T_OP_MODULO
};


static SExpression* parse(int precedence);


static SExpression*
parseInitial(void) {
	switch (lex_Context->token.id) {
		case T_NUMBER: {
			SExpression* expr = expr_Const(lex_Context->token.value.integer);
			parse_GetToken();
			return expr;
		}
		case T_AT: {
			parse_GetToken();
			return expr_Pc();
		}
		case T_ID: {
			internalerror("not implemented");
			break;
		}
	}

	return NULL;
}


static SExpression*
parseUnary(void) {
	switch (lex_Context->token.id) {
		case T_OP_ADD: {
			parse_GetToken();
			return parse_Expression(8);
		}
		case T_OP_SUBTRACT: {
			parse_GetToken();
			return expr_Sub(expr_Const(0), parse_Expression(8));
		}
		case T_OP_BITWISE_NOT: {
			parse_GetToken();
            return expr_Xor(expr_Const(0xFFFFFFFF), parse_Expression(8));
		}
		case T_OP_BOOLEAN_NOT: {
			parse_GetToken();
            return expr_BooleanNot(parse_Expression(8));
		}
		default: {
			return parseInitial();
		}
	}
}


static SExpression*
parseBinaryOperator(SExpression* lhs) {
	switch (lex_Context->token.id) {
		case T_OP_ADD: {
			parse_GetToken();
			return expr_Add(lhs, parse(PRECEDENCE_ADD));
		}
		default: {
			internalerror("Token not implemented in parseBinaryOperator");
		}
	}
}


static SExpression*
parse(int precedence) {
	SExpression* left = parseUnary();
	if (left == NULL)
		return NULL;

	uint32_t token = lex_Context->token.id - FIRST_PRECEDENCE_TOKEN;
	while (token < TOTAL_PRECEDENCES && precedence < s_precedence[token]) {
		left = parseBinaryOperator(left);
		token = lex_Context->token.id - FIRST_PRECEDENCE_TOKEN;
	}

	return left;
}


extern SExpression*
parse_Expression(size_t maxStringConstLength) {
	return parse(0);
}


extern int64_t
parse_ConstantExpression(size_t maxStringConstLength) {
	internalerror("parse_ConstantExpression not implemented");
}
