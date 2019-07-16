#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <fcntl.h>
#include "ast.h"

//the below code is heavily based on the calculator code
//from Nat's notes

calc_ast* make_ast_value(svec* prompt) {
	calc_ast* ast = malloc(sizeof(calc_ast));

	//'=' indicates that this node is a value that can be evaluated
	//as a unique process
	ast->op = "=";
	ast->func = 0;
	if (prompt->size > 0) {
		//if there were commands/instructions passed to this node
		//then initialize it with that
		ast->func = strdup(prompt->data[0]);
	}
	ast->arguments = make_svec();

	for (int ii = 0; ii < prompt->size; ii++) {
		svec_push(ast->arguments, prompt->data[ii]);
	}

	ast->arg0 = 0;
	ast->arg1 = 0;

	return ast;
}
calc_ast* make_ast_op(char* op, calc_ast* a0, calc_ast* a1) {
	calc_ast* ast = malloc(sizeof(calc_ast));

	//initialize the tree with the operation that defines its node
	ast->op = op;
	ast->arg0 = a0;
	ast->arg1 = a1;
	//since this node is not a process that can be evaluated (instead represents operator),
	//func and arguments are irrelevant
	ast->func = 0;
	ast->arguments = 0;

	return ast;
}

void free_ast(calc_ast* ast) {
	if (strcmp(ast->op, "=") != 0) {
		free_ast(ast->arg0);
		free_ast(ast->arg1);
	} else {
		free(ast->func);
		free_svec(ast->arguments);
	}

	free(ast);
}

//this function is the base case for recursive calls:
//initialize process to new process based on AST
int execute(calc_ast* ast) {
	int cpid;
	if ((cpid = fork())) {
		int status;
		waitpid(cpid, &status, 0);
		return status;
	}
	else {
		ast->arguments->data[ast->arguments->size] = 0;
		execvp(ast->func, ast->arguments->data);
	}

	//dummy value, shouldn't happen
	return -1;
}

//the function for the input redirection operator
int input_redirect(calc_ast* ast) {
	int cpid;

	if ((cpid = fork())) {
		int result;
		waitpid(cpid, &result, 0);
		return result;
	} 
	else {
		//save stdin, close it, duplicate file,
		//do reading and execution and such,
		//then restore stdin and return result from child process
		int fd = open(ast->arg1->func, 0);
		int new_in = dup(0);
		close(0);
		dup(fd);
		close(fd);
		int new_result = ast_eval(ast->arg0);
		close(0);
		dup(new_in);
		return new_result;
	}

	//dummy value
	return -1;
}

//the function for the output redirection operator
int output_redirect(calc_ast* ast) {
	int cpid;

	if ((cpid = fork())) {
		int result;
		waitpid(cpid, &result, 0);
		return result;
	} else {
		//open the file, save stdout, close stdout, duplicate file,
		//execute new process, then restore stdout
		//and return result of child process
		int fd = open(ast->arg1->func, O_CREAT | O_WRONLY | O_TRUNC, 0644);
		int new_result = dup(1);
		close(1);
		dup(fd);
		close(fd);


		int solution = ast_eval(ast->arg0);
		close(1);
		dup(new_result);
		return solution;
	}
	
	//dummy value
	return -1;
}

//function for establishing and executing pipe
int piping(calc_ast* ast) {
	int cpid;

	if ((cpid = fork())) {
		int status;
		waitpid(cpid, &status, 0);
		return status;
	} else {
		int pipes[2];
		pipe(pipes);
		int nextpid;

		if ((nextpid = fork())) {
			int another_status;
			close(pipes[1]);
			close(0);
			dup(pipes[0]);
			ast_eval(ast->arg1);
			waitpid(nextpid, &another_status, 0);
		} else {
			close(pipes[0]);	
			close(1);
			dup(pipes[1]);
			ast_eval(ast->arg0);
		}

	}

	//dummy value
	return -1;
}

//evaluates the commands stored in the tree
//by checking the operator in the root node
//and calling functions and executing things accordingly
int ast_eval(calc_ast* ast) {
	if (strcmp(ast->op, "=") == 0) {
		if (strcmp(ast->func, "cd") == 0) {
			return chdir(ast->arguments->data[1]);
		}
		if (strcmp(ast->func, "exit") == 0) {
			exit(0);
		}
		return execute(ast);
	}
	else if (strcmp(ast->op, ";") == 0) {
		ast_eval(ast->arg0);
		return ast_eval(ast->arg1);
	}
	else if (strcmp(ast->op, "&&") == 0) {
		int rv = ast_eval(ast->arg0);
		if (rv == 0) {
			return ast_eval(ast->arg1);	
		}
		return rv;
	}  
	else if (strcmp(ast->op, "||") == 0) {
		int rv = ast_eval(ast->arg0);
		if (rv != 0) {
			return ast_eval(ast->arg1);
		}
		return rv;
	}
	else if (strcmp(ast->op, "<") == 0) {
		return input_redirect(ast);
	}
	else if (strcmp(ast->op, ">") == 0) {
		return output_redirect(ast);
	}
	else if (strcmp(ast->op, "|") == 0) {
		return piping(ast);
	}
	else if (strcmp(ast->op, "&") == 0) {
		int cpid;

		if ((cpid = fork())) {
			if (ast->arg1->func != 0) {
				return ast_eval(ast->arg1);
			}
			else {
				return 0;
			}
		} else {
			ast_eval(ast->arg0);
		}
	}
	else {
		return 0;
	}

	//dummy value
	return -1;
}

