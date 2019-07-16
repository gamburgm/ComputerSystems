#ifndef AST_H
#define AST_H

#include "svec.h"

//the below code is heavily based on the AST from the calculator example
//from Nat's notes
typedef struct calc_ast {
	char* op;
	char* func;
	svec* arguments;
	//op is one of:
	// &, &&, |, ||, ;, <, >
	// '=', this is something that needs evaluating
	struct calc_ast* arg0;
	struct calc_ast* arg1;
} calc_ast;


calc_ast* make_ast_value(svec* vv);
calc_ast* make_ast_op(char* op, calc_ast* a0, calc_ast* a1);
void free_ast(calc_ast* ast);
int ast_eval(calc_ast* ast);

#endif
