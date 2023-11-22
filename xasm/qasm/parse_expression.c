#include "errors.h"
#include "lexer.h"
#include "parse.h"
#include "parse_expression.h"


#define FIRST_PRECEDENCE_TOKEN T_OP_EQUAL
#define TOTAL_PRECEDENCES (T_OP_MODULO - FIRST_PRECEDENCE_TOKEN + 1)

#define PRECEDENCE_BOOLEAN_LOGIC 1
#define PRECEDENCE_COMPARE 2
#define PRECEDENCE_ADD 3
#define PRECEDENCE_MUL 4
#define PRECEDENCE_BITWISE_LOGIC 5
#define PRECEDENCE_UNARY 6		// Not necessary, but here for documentation

static int s_precedence[] = {
	PRECEDENCE_COMPARE,			// T_OP_EQUAL
	PRECEDENCE_COMPARE,			// T_OP_NOT_EQUAL
	PRECEDENCE_COMPARE,			// T_OP_GREATER_THAN
	PRECEDENCE_COMPARE,			// T_OP_GREATER_OR_EQUAL
	PRECEDENCE_COMPARE,			// T_OP_LESS_THAN
	PRECEDENCE_COMPARE,			// T_OP_LESS_OR_EQUAL

	PRECEDENCE_BOOLEAN_LOGIC,	// T_OP_BOOLEAN_OR
	PRECEDENCE_BOOLEAN_LOGIC,	// T_OP_BOOLEAN_AND

	PRECEDENCE_UNARY,			// T_OP_BOOLEAN_NOT

	PRECEDENCE_BITWISE_LOGIC,	// T_OP_BITWISE_OR
	PRECEDENCE_BITWISE_LOGIC,	// T_OP_BITWISE_XOR
	PRECEDENCE_BITWISE_LOGIC,	// T_OP_BITWISE_AND

	PRECEDENCE_MUL,				// T_OP_BITWISE_ASL
	PRECEDENCE_MUL,				// T_OP_BITWISE_ASR

	PRECEDENCE_UNARY,			// T_OP_BITWISE_NOT

	PRECEDENCE_ADD,				// T_OP_ADD
	PRECEDENCE_ADD,				// T_OP_SUBTRACT

	PRECEDENCE_MUL,				// T_OP_MULTIPLY
	PRECEDENCE_MUL,				// T_OP_DIVIDE
	PRECEDENCE_MUL,				// T_OP_MODULO
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
		case T_STRING: {
			string* str = lex_TokenString();
			SExpression* expr = expr_String(str);
			str_Free(str);
			parse_GetToken();
			return expr;
		}
		case '@': {
			parse_GetToken();
			return expr_Pc();
		}
		case '(': {
			parse_GetToken();
			SExpression* expr = parse(0);
			if (lex_Context->token.id == ')') {
				parse_GetToken();
				return expr;
			}

			expr_Free(expr);
			err_Error(ERROR_EXPECTED_CHAR, ',');
			return NULL;
		}
		case T_ID: {
			string* str = str_CreateLength(lex_Context->token.value.string, lex_Context->token.length);
			SExpression* expr = expr_SymbolByName(str);

			str_Free(str);
			parse_GetToken();

			return expr;
		}
	}

	err_Error(ERROR_INVALID_EXPRESSION);
	return NULL;
}


static SExpression*
parseUnary(void) {
	switch (lex_Context->token.id) {
		case T_OP_ADD: {
			parse_GetToken();
			return parse(0);
		}
		case T_OP_SUBTRACT: {
			parse_GetToken();
			return expr_Sub(expr_Const(0), parse(0));
		}
		case T_OP_BITWISE_NOT: {
			parse_GetToken();
            return expr_Xor(expr_Const(0xFFFFFFFF), parse(0));
		}
		case T_OP_BOOLEAN_NOT: {
			parse_GetToken();
            return expr_BooleanNot(parse(0));
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
		case T_OP_SUBTRACT: {
			parse_GetToken();
			return expr_Sub(lhs, parse(PRECEDENCE_ADD));
		}
		case T_OP_MULTIPLY: {
			parse_GetToken();
			return expr_Mul(lhs, parse(PRECEDENCE_MUL));
		}
		case T_OP_DIVIDE: {
			parse_GetToken();
			return expr_Div(lhs, parse(PRECEDENCE_MUL));
		}
		case T_OP_MODULO: {
			parse_GetToken();
			return expr_Mod(lhs, parse(PRECEDENCE_MUL));
		}
		case T_OP_BITWISE_ASL: {
			parse_GetToken();
			return expr_Asl(lhs, parse(PRECEDENCE_MUL));
		}
		case T_OP_BITWISE_ASR: {
			parse_GetToken();
			return expr_Asr(lhs, parse(PRECEDENCE_MUL));
		}
		case T_OP_BITWISE_AND: {
			parse_GetToken();
			return expr_And(lhs, parse(PRECEDENCE_BITWISE_LOGIC));
		}
		case T_OP_BITWISE_OR: {
			parse_GetToken();
			return expr_Or(lhs, parse(PRECEDENCE_BITWISE_LOGIC));
		}
		case T_OP_BITWISE_XOR: {
			parse_GetToken();
			return expr_Xor(lhs, parse(PRECEDENCE_BITWISE_LOGIC));
		}
		case T_OP_BOOLEAN_AND: {
			parse_GetToken();
			return expr_BooleanAnd(lhs, parse(PRECEDENCE_BOOLEAN_LOGIC));
		}
		case T_OP_BOOLEAN_OR: {
			parse_GetToken();
			return expr_BooleanOr(lhs, parse(PRECEDENCE_BOOLEAN_LOGIC));
		}
		case T_OP_EQUAL: {
			parse_GetToken();
			return expr_Equal(lhs, parse(PRECEDENCE_COMPARE));
		}
		case T_OP_NOT_EQUAL: {
			parse_GetToken();
			return expr_NotEqual(lhs, parse(PRECEDENCE_COMPARE));
		}
		case T_OP_LESS_THAN: {
			parse_GetToken();
			return expr_LessThan(lhs, parse(PRECEDENCE_COMPARE));
		}
		case T_OP_GREATER_THAN: {
			parse_GetToken();
			return expr_GreaterThan(lhs, parse(PRECEDENCE_COMPARE));
		}
		case T_OP_LESS_OR_EQUAL: {
			parse_GetToken();
			return expr_LessEqual(lhs, parse(PRECEDENCE_COMPARE));
		}
		case T_OP_GREATER_OR_EQUAL: {
			parse_GetToken();
			return expr_GreaterEqual(lhs, parse(PRECEDENCE_COMPARE));
		}
		default: {
			internalerror("Token not implemented in parseBinaryOperator");
		}
	}
}


static SExpression*
parse(int precedence) {
	SExpression* left = parseUnary();
	uint32_t token = lex_Context->token.id - FIRST_PRECEDENCE_TOKEN;

	while (left != NULL && token < TOTAL_PRECEDENCES && precedence < s_precedence[token]) {
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
	SExpression* expr = parse_Expression(maxStringConstLength);
	if (expr->isConstant) {
		int64_t v = expr->value.integer;
		expr_Free(expr);
		return v;
	}
	err_Error(ERROR_EXPR_CONST);
	return 0;
}


extern string*
parse_StringExpression(size_t maxStringConstLength) {
	SExpression* expr = parse_Expression(maxStringConstLength);
	if (expr->type == EXPR_STRING) {
		string* str = str_Copy(expr->value.string);
		expr_Free(expr);
		return str;
	}
	err_Error(ERROR_EXPR_STRING);
	return str_Empty();
} 
