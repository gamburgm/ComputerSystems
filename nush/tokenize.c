#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "tokenize.h"

//all of the below code is based on Nat's lecture notes (for calculator)
int isop(char in) {
	//easy way to check if found an operator for nush
	//used in the same way that isspace or isalpha is used
	return (in == '&') || (in == '|') || (in == '<')
		|| (in == '>') || (in == ';');
}

char* read_cmd(char* text, int ii) {
	int nn = 0;

	while (!isop(text[ii + nn]) && !isspace(text[ii + nn])) {
		nn++;
	}

	char* cmd = malloc(nn + 1);
	memcpy(cmd, text + ii, nn);
	cmd[nn] = 0;
	return cmd;
}

char* read_op(char* text, int ii) {
	int doubleChar = 0;
	char* op = malloc(3);

	//if you find an operator, create a string representation for it and return that
	switch (text[ii]) {
		case '<': case '>': case ';': 
			op[0] = text[ii++];	
			break;
		case '|': case '&':
			op[0] = text[ii];
			if (text[ii + 1] == text[ii]) {
				op[1] = text[ii++];
				doubleChar = 1;
			}
			ii++;
			break;
		default:
			break;
	}

	op[doubleChar + 1] = 0;
	
	return op;
}
/*
Operators that have to be handled here:
'>'
'<'
'|'
'&'
'&&'
'||'
';'
*/

svec* tokenize(char* text) {
	svec* xs = make_svec();
	
	int nn = strlen(text);
	int ii = 0;

	while (ii < nn) {
		//skip spaces
		if (isspace(text[ii])) {
			ii++;
			continue;
		}
		//if found operator, extract operator, push it to svec, and continue
		if (isop(text[ii])) {
			char* op = read_op(text, ii);
			ii += strlen(op);
			svec_push(xs, op);
			free(op);
			continue;
		} 

		//else, must be a regular command of alpha-numeric characters
		//read and tokenize it, push to svec, and continue
		char* cmd = read_cmd(text, ii);
		ii += strlen(cmd);
		svec_push(xs, cmd);
		free(cmd);
	}

	return xs;
}


