#ifndef SVEC_H
#define SVEC_H

//code from Nat's notes
typedef struct svec {
	int size;
	int cap;
	char** data;
} svec;

svec* make_svec();

void free_svec(svec*);

char* svec_get(svec* sv, int ii);

void svec_put(svec* sv, int ii, char* item);

void svec_push(svec* sv, char* item);


#endif
