
/* LORE LOOP NESTS -- Functions having atleast one loop nest without Mem write */
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


// 1D-Sum
// loop peeling
int ex14()
{
  int sum = 0;
	for (int j = 0; j < LEN ; j++) {
    if(j < 3)
      sum += 2*b[j];
    else 
		  sum += b[j];
	}
	return sum;
}



//loop unswitching, distribution 1D loop
//Sum
int ex15()
{

//	control flow
//	if test using loop index
  int sum = 0;
  int len = 16384;
	int mid = (len/2);
		for (int i = 0; i < len; i++) {
			if (i < mid) 
				sum += b[2*i];
			if (i >= mid) 
				sum += c[i];
		}
	return sum;
}

//loop unswitching, distribution 1D loop
//Sum
////8 uf
int ex15_8()
{

//	control flow
//	if test using loop index
  int sum = 0;
  int len = 16384;
	int mid = (LEN/2);
		for (int i = 0; i < LEN; i++) {
			if (i < mid) 
				sum += c[a[i]];
			if (i >= mid) 
				sum += b[i];
		}
	return sum;
}


//loop unroll complete 1D
//Sum
int ex16()
{

  int sum1 = 0;
	for (int j = 0; j < 3; j++) 
	  sum1 += a[j];
	for (int j = 3; j < LEN; j++) 
	  sum1 += b[j];
	return sum1;
}


//remainder loop  fusion
//Sum
int ex17(int n)
{

  int sum1 = 0;
  int sum2 = 0;
	for (int i = 0; i < n; i++) {
		sum1 += a[i];
	}
	for (int i = 0; i < n; i++) {
		sum2 += b[i];
	}
	return sum1 + sum2;
}


int main(){

	return 0;
}

