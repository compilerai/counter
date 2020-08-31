
/* LORE LOOP NESTS -- Functions in which all loop nests have at leat one memory write/update */
// This file contains the C source code for functions which are either not auto-vectorized or generate non-bismilar assembly auto-vectorization
// The optimized/transformed C source code for these functions is attached in corr_mutations_dst.c file, which are then auto-vectorized by GCC and Clang


#define LEN 32000
#define LEN1 3200
#define LEN2 2048

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/times.h>
#include <sys/types.h>
#include <time.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "eqchecker_helper.h"

#define TYPE int
__attribute__((aligned(16))) TYPE a[LEN],b[LEN],c[LEN],d[LEN],e[LEN],
                                   aa[LEN2][LEN2],bb[LEN2][LEN2],cc[LEN2][LEN2],tt[LEN2][LEN2];

// 1D-MW
// loop peeling
int ex109()
{

	for (int j = 0; j < LEN ; j++) {
    if(j < 3)
      b[j] = 0;
    else 
		  b[j] = a[j];
	}
	return 0;
}

// 1D-MW
// loop peeling
// 8 uf
int ex109_8()
{

	for (int j = 0; j < LEN ; j++) {
    if(j < 3)
      a[j] = 100;
    else 
		  a[j] = b[j]+2;
	}
	return 0;
}


//loop unswitching, distribution 1D loop
// MW
int ex1010()
{

//	control flow
//	if test using loop index

	int mid = (LEN/2);
		for (int i = 0; i < LEN; i++) {
			if (i < mid) 
				a[i] += b[i];
			if (i >= mid) 
				a[i] += c[i];
		}
	return 0;
}


//loop unroll complete 1D
//MW
int ex1011()
{

  int sum1 = 0;
	for (int j = 0; j < 3; j++) 
	  c[j] += a[j];
	for (int j = 3; j < LEN; j++) 
	  c[j] += b[j];
	return 0;
}




int main(){

	return 0;
}

