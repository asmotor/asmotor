#pragma once

typedef enum {
	T_NONE = 0,

	T_FIRST_TOKEN = 300,

	T_STRING = T_FIRST_TOKEN,
	T_LABEL,
	T_ID,
	T_NUMBER,
	T_FLOAT,

	T_OP_EQUAL,
	T_OP_NOT_EQUAL,
	T_OP_GREATER_THAN,
	T_OP_GREATER_OR_EQUAL,
	T_OP_LESS_THAN,
	T_OP_LESS_OR_EQUAL,

	T_OP_BOOLEAN_OR,
	T_OP_BOOLEAN_AND,
	T_OP_BOOLEAN_NOT,
	T_OP_BITWISE_OR,
	T_OP_BITWISE_XOR,
	T_OP_BITWISE_AND,
	T_OP_BITWISE_ASL,
	T_OP_BITWISE_ASR,
	T_OP_BITWISE_NOT,

	T_OP_ADD,
	T_OP_SUBTRACT,
	T_OP_MULTIPLY,
	T_OP_DIVIDE,
	T_OP_MODULO,

	T_SYM_MACRO,
	T_SYM_CONSTANT,
	T_SYM_GROUP,

} token_t;


extern void
tokens_Define(void);
