#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "svec.h"
#include "tokenize.h"
#include "parse.h"
#include "ast.h"


int main(int argc, char* argv[]) {
	//code from main, including fgets, and using line with it, are based on
	//the calculator lecture

	char line[100];

	//if there is a file passed into argv, open and use it
	if (argc == 2) {
		FILE* file = fopen(argv[1], "r");
		char* rv;

		while ((rv = fgets(line, 96, file))) {
			fflush(file);

			svec* toks = tokenize(line);
			calc_ast* tree = parse(toks);
			ast_eval(tree);

			free_svec(toks);
			free_ast(tree);
		}

		fclose(file);
		exit(0);
	}

	//otherwise, do a regular loop, printing nush
	while (1) {
		printf("nush$ ");
		fflush(stdout);

		char* rv = fgets(line, 96, stdin);
		fflush(stdin);
		if (rv == 0) {
			exit(0);
		}


		svec* toks = tokenize(line);
		calc_ast* tree = parse(toks);
		ast_eval(tree);


		free_svec(toks);
		free_ast(tree);
	}

	return 0;
}




