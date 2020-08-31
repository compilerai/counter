
/* LORE LOOP NESTS -- Functions in which all loop nests have at leat one memory write/update */
// This file contains the optimized/transformed C source code for functions in corr_mutations_src.c file
// The optimized/transformed C source code attached in this file is auto-vectorized by GCC and Clang

#define LEN 32000
#define LEN1 3200
#define LEN2 256

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

  b[0] = 0;
  b[1] = 0;
  b[2] = 0;
	for (int j = 3; j < LEN ; j++) {
		  b[j] = a[j];
	}
	return 0;
}

// 1D-MW
// loop peeling
// 8 uf
int ex109_8()
{

  a[0] = 100;
  a[1] = 100;
  a[2] = 100;
  #pragma GCC unroll 2
	for (int j = 3; j < LEN ; j++) {
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
		for (int i = 0; i < mid; i++) {
				a[i] += b[i];
    }
		for (int i = mid; i < LEN; i++) {
				a[i] += c[i];
		}
	return 0;
}


//loop unroll complete 1D
//MW
int ex1011()
{

  int sum1 = 0;
	c[0] += a[0];
	c[1] += a[1];
	c[2] += a[2];
	for (int j = 3; j < LEN; j++) 
	  c[j] += b[j];
	return 0;
}

int main(){

	return 0;
}

