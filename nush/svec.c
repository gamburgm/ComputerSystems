#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "svec.h"

//code from Nat's notes
svec* make_svec() {
	svec* xs = malloc(sizeof(svec));
	xs->size = 0;
	xs->cap = 4;
	xs->data = malloc(xs->cap * sizeof(char*));
	return xs;
}


//should use copies for getting and putting/pushing
//to prevent accidentally freeing other users' strings
void free_svec(svec* xs) {
	for (int ii = 0; ii < xs->size; ii++) {
		free(xs->data[ii]);
	}

	free(xs->data);

	free(xs);
}

char* svec_get(svec* sv, int ii) {
	//using assert is awesome, do that
	assert(ii >= 0 && ii < sv->size);
	return strdup(sv->data[ii]);
}

void svec_put(svec* sv, int ii, char* item) {
	assert(ii >= 0 && ii < sv->size);
	free(sv->data[ii]);
	sv->data[ii] = strdup(item);
}

void svec_push(svec* sv, char* item) {
	if (sv->cap <= sv->size) {
		sv->cap *= 2;
		sv->data = realloc(sv->data, sv->cap * sizeof(char*));
	}

	sv->data[sv->size] = strdup(item);
	sv->size += 1;
}


