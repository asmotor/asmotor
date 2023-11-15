#include "mem.h"

#include "errors.h"
#include "expression.h"
#include "symbol.h"


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


static SExpression*
unaryIntegerOperation(SExpression* left, token_t operation, int64_t (*op)(int64_t)) {
	SExpression* expr = allocExpression(EXPR_OPERATION);
	expr->operation = operation;
	expr->left = left;
	expr->right = NULL;
	expr->isConstant = left->isConstant;
	if (left->isConstant)
		expr->value.integer = op(left->value.integer);

	return expr;
}


static int64_t op_add(int64_t lhs, int64_t rhs) { return lhs + rhs; }
static int64_t op_sub(int64_t lhs, int64_t rhs) { return lhs - rhs; }
static int64_t op_mul(int64_t lhs, int64_t rhs) { return lhs * rhs; }
static int64_t op_div(int64_t lhs, int64_t rhs) { return lhs / rhs; }
static int64_t op_mod(int64_t lhs, int64_t rhs) { return lhs % rhs; }
static int64_t op_asl(int64_t lhs, int64_t rhs) { return lhs << rhs; }
static int64_t op_asr(int64_t lhs, int64_t rhs) { return lhs >> rhs; }
static int64_t op_bitwise_and(int64_t lhs, int64_t rhs) { return lhs & rhs; }
static int64_t op_bitwise_or(int64_t lhs, int64_t rhs) { return lhs | rhs; }
static int64_t op_bitwise_xor(int64_t lhs, int64_t rhs) { return lhs ^ rhs; }
static int64_t op_boolean_and(int64_t lhs, int64_t rhs) { return lhs && rhs; }
static int64_t op_boolean_or(int64_t lhs, int64_t rhs) { return lhs || rhs; }
static int64_t op_boolean_not(int64_t v) { return !v; }
static int64_t op_equal(int64_t lhs, int64_t rhs) { return lhs == rhs; }
static int64_t op_not_equal(int64_t lhs, int64_t rhs) { return lhs != rhs; }
static int64_t op_greater_than(int64_t lhs, int64_t rhs) { return lhs > rhs; }
static int64_t op_less_than(int64_t lhs, int64_t rhs) { return lhs < rhs; }
static int64_t op_greater_or_equal(int64_t lhs, int64_t rhs) { return lhs >= rhs; }
static int64_t op_less_or_equal(int64_t lhs, int64_t rhs) { return lhs <= rhs; }


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
		expr_Free(expression);
		return NULL;
	}

	return expression;
}


extern SExpression*
expr_BooleanNot(SExpression* expr) {
	return unaryIntegerOperation(expr, T_OP_BOOLEAN_NOT, op_boolean_not);
}


extern SExpression*
expr_BooleanOr(SExpression* left, SExpression* right) {
	return combineIntegerOperation(left, right, T_OP_BOOLEAN_OR, op_boolean_or);
}


extern SExpression*
expr_BooleanAnd(SExpression* left, SExpression* right) {
	return combineIntegerOperation(left, right, T_OP_BOOLEAN_AND, op_boolean_and);
}


extern SExpression*
expr_And(SExpression* left, SExpression* right) {
	return combineIntegerOperation(left, right, T_OP_BITWISE_AND, op_bitwise_and);
}


extern SExpression*
expr_Or(SExpression* left, SExpression* right) {
	return combineIntegerOperation(left, right, T_OP_BITWISE_OR, op_bitwise_or);
}


extern SExpression*
expr_Xor(SExpression* left, SExpression* right) {
	return combineIntegerOperation(left, right, T_OP_BITWISE_XOR, op_bitwise_xor);
}


extern SExpression*
expr_Add(SExpression* left, SExpression* right) {
	return combineIntegerOperation(left, right, T_OP_ADD, op_add);
}


extern SExpression*
expr_Sub(SExpression* left, SExpression* right) {
	return combineIntegerOperation(left, right, T_OP_SUBTRACT, op_sub);
}


extern SExpression*
expr_Mul(SExpression* left, SExpression* right) {
	return combineIntegerOperation(left, right, T_OP_MULTIPLY, op_mul);
}


extern SExpression*
expr_Div(SExpression* left, SExpression* right) {
	return combineIntegerOperation(left, right, T_OP_DIVIDE, op_div);
}


extern SExpression*
expr_Mod(SExpression* left, SExpression* right) {
	return combineIntegerOperation(left, right, T_OP_MODULO, op_mod);
}


extern SExpression*
expr_Asl(SExpression* left, SExpression* right) {
	return combineIntegerOperation(left, right, T_OP_BITWISE_ASL, op_asl);
}


extern SExpression*
expr_Asr(SExpression* left, SExpression* right) {
	return combineIntegerOperation(left, right, T_OP_BITWISE_ASR, op_asr);
}


extern SExpression*
expr_PcRelative(SExpression* expr, int adjustment) {
	internalerror("expr_PcRelative not implemented");
	return NULL;
}


extern SExpression*
expr_Equal(SExpression* left, SExpression* right) {
	return combineIntegerOperation(left, right, T_OP_EQUAL, op_equal);
}


extern SExpression*
expr_NotEqual(SExpression* left, SExpression* right) {
	return combineIntegerOperation(left, right, T_OP_NOT_EQUAL, op_not_equal);
}


extern SExpression*
expr_GreaterThan(SExpression* left, SExpression* right) {
	return combineIntegerOperation(left, right, T_OP_GREATER_THAN, op_greater_than);
}


extern SExpression*
expr_LessThan(SExpression* left, SExpression* right) {
	return combineIntegerOperation(left, right, T_OP_LESS_THAN, op_less_than);
}


extern SExpression*
expr_GreaterEqual(SExpression* left, SExpression* right) {
	return combineIntegerOperation(left, right, T_OP_GREATER_OR_EQUAL, op_greater_or_equal);
}


extern SExpression*
expr_LessEqual(SExpression* left, SExpression* right) {
	return combineIntegerOperation(left, right, T_OP_LESS_OR_EQUAL, op_less_or_equal);
}


extern SExpression*
expr_Pc(void) {
	internalerror("expr_Pc not implemented");
	return NULL;
}


extern SExpression*
expr_SymbolByName(const string* name) {
	SSymbol* sym = sym_Find(name);
	if (sym->type == SYMBOL_INTEGER_CONSTANT || sym->type == SYMBOL_INTEGER_VARIABLE) {
		return expr_Const(sym->value.integer);
	}

	SExpression* expr = allocExpression(EXPR_SYMBOL);
	expr->value.symbol = sym;

	return expr;
}
