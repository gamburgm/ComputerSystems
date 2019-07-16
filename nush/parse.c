#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse.h"


//from Nat's notes --> the calculator
int streq(const char* aa, const char* bb) {
	return strcmp(aa, bb) == 0;
}

//from Nat's notes --> the calculator
int find_first_index(svec* toks, const char* tt) {
	for (int ii = 0; ii < toks->size; ii++) {
		if (streq(toks->data[ii], tt)) {
			return ii;
		}
	}
	return -1;
}

//from Nat's notes --> the calculator
int contains(svec* toks, const char* tt) {
	return find_first_index(toks, tt) != -1;
}

//from Nat's notes --> the calculator
svec* slice(svec* xs, int i0, int i1) {
	svec* thing = make_svec();
	for (int ii = i0; ii < i1; ii++) {
		svec_push(thing, xs->data[ii]);
	}
	return thing;
}

int has_op(svec* toks) {
	const char* ops[] = {"&", "||", "|", "&&", "<", ">", ";"};

	for (int ii = 0; ii < toks->size; ii++) {
		for (int jj = 0; jj < 7; jj++) {
			if (streq(toks->data[ii], ops[jj])) {
				return 1;
			}
		}
	}

	return 0;
}

//based on the parse function from the calculator in Nat's notes
calc_ast* parse(svec* toks) {
	char* ops[] = {";", "&", "&&", "|", "||", "<", ">"};

	//for the tokens, construct a tree by creating branches around certain operators
	//the 'order of operations' is defined by the above array (ops)
	for (int ii = 0; ii < 7; ii++) {
		 char* op = ops[ii];
		
		if (contains(toks, op)) {
			int jj = find_first_index(toks, op);
			svec* xs = slice(toks, 0, jj);
			svec* ys = slice(toks, jj + 1, toks->size);
			calc_ast* ast = make_ast_op(ops[ii], parse(xs), parse(ys));
			free(xs);
			free(ys);
			return ast;
		}
	}

	return make_ast_value(toks);
}








