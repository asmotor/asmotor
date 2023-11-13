#include "mem.h"

#include "errors.h"
#include "expression.h"


static SExpression*
allocExpression(exprtype_t type) {
	SExpression* expr = (SExpression*) mem_Alloc(sizeof(SExpression));
	expr->left = NULL;
	expr->right = NULL;
	expr->type = type;
	expr->isConstant = false;
	expr->value.symbol = NULL;

	return expr;
}


static SExpression*
combineIntegerOperation(SExpression* left, SExpression* right, token_t operation, int64_t (*op)(int64_t, int64_t)) {
	if (left == NULL || right == NULL) {
		expr_Free(left);
		expr_Free(right);
		return NULL;
	}

	SExpression* expr = allocExpression(EXPR_OPERATION);
	expr->operation = operation;
	expr->left = left;
	expr->right = right;
	expr->isConstant = left->isConstant && right->isConstant;
	if (expr->isConstant)
		expr->value.integer = op(left->value.integer, right->value.integer);

	return expr;
}


static int64_t op_add(int64_t lhs, int64_t rhs) { return lhs + rhs; }
static int64_t op_bitwise_and(int64_t lhs, int64_t rhs) { return lhs & rhs; }


extern void
expr_Free(SExpression* expr) {
	if (expr != NULL) {
		expr_Free(expr->left);
		expr_Free(expr->right);

		mem_Free(expr);
	}
}


extern SExpression*
expr_Const(int64_t value) {
	SExpression* expr = allocExpression(EXPR_CONSTANT);
	expr->isConstant = true;
	expr->value.integer = value;

	return expr;
}


extern SExpression*
expr_CheckRange(SExpression* expression, int64_t low, int64_t high) {
	if (expression == NULL)
		return NULL;

	if (expression->isConstant) {
		if (expression->value.integer >= low && expression->value.integer <= high) {
			return expression;
		}
		err_Error(ERROR_INTEGER_RANGE, expression->value.integer, low, high);
		expr_Free(expression);
		return NULL;
	}

	return NULL;
}


extern void
sect_OutputExpr8(struct Expression* expr) {
	printf("%02lX ", expr->value.integer);
}


extern void
sect_OutputExpr16(struct Expression* expr) {
	internalerror("sect_OutputExpr16 not implemented");
}


extern void
sect_OutputExpr32(struct Expression* expr) {
	internalerror("sect_OutputExpr32 not implemented");
}


extern SExpression*
expr_BooleanNot(SExpression* expr) {
	internalerror("expr_BooleanNot not implemented");
	return NULL;
}


extern SExpression*
expr_BooleanOr(SExpression* left, SExpression* right) {
	internalerror("expr_BooleanOr not implemented");
	return NULL;
}


extern SExpression*
expr_BooleanAnd(SExpression* left, SExpression* right) {
	internalerror("expr_BooleanAnd not implemented");
	return NULL;
}


extern SExpression*
expr_And(SExpression* left, SExpression* right) {
	return combineIntegerOperation(left, right, T_OP_BITWISE_AND, op_bitwise_and);
}


extern SExpression*
expr_Or(SExpression* left, SExpression* right) {
	internalerror("expr_Or not implemented");
	return NULL;
}


extern SExpression*
expr_Xor(SExpression* left, SExpression* right) {
	internalerror("expr_Xor not implemented");
	return NULL;
}


extern SExpression*
expr_Add(SExpression* left, SExpression* right) {
	return combineIntegerOperation(left, right, T_OP_ADD, op_add);
}


extern SExpression*
expr_Sub(SExpression* left, SExpression* right) {
	internalerror("expr_Sub not implemented");
	return NULL;
}


extern SExpression*
expr_Asl(SExpression* left, SExpression* right) {
	internalerror("expr_Asl not implemented");
	return NULL;
}


extern SExpression*
expr_Asr(SExpression* left, SExpression* right) {
	internalerror("expr_Asr not implemented");
	return NULL;
}


extern SExpression*
expr_PcRelative(SExpression* expr, int adjustment) {
	internalerror("expr_PcRelative not implemented");
	return NULL;
}


extern SExpression*
expr_Pc(void) {
	internalerror("expr_Pc not implemented");
	return NULL;
}

