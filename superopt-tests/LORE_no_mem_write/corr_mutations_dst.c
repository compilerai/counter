
/* LORE LOOP NESTS -- Functions having atleast one loop nest without Mem write */
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

// 1D-Sum
// loop peeling
int ex14()
{
  int sum = 0;
  sum += 2*b[0];
  sum += 2*b[1];
  sum += 2*b[2];
	for (int j = 3; j < LEN ; j++) {
		  sum += b[j];
	}
	return sum;
}


//loop unswitching, distributiona 1D loop
//Sum
int ex15()
{

//	control flow
//	if test using loop index
  int sum = 0;
  int len = 16384;
	int mid = (len/2);
		for (int i = 0; i < mid; i++) {
				sum += b[2*i];
    }
		for (int i = mid; i < len; i++) {
				sum += c[i];
		}
	return sum;
}

//loop unswitching, distributiona 1D loop
//Sum
//8 uf
int ex15_8()
{

//	control flow
//	if test using loop index
  int sum = 0;
	int mid = (LEN/2);
		for (int i = 0; i < mid; i++) {
				sum += c[a[i]];
    }
  #pragma GCC unroll 2
		for (int i = mid; i < LEN; i++) {
				sum += b[i];
		}
	return sum;
}

//loop unroll complete 1D
//Sum
int ex16()
{

  int sum1 = 0;
	sum1 += a[0];
	sum1 += a[1];
	sum1 += a[2];
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
  if( n < 4)
  {
    if(n >= 1) {  sum1 += a[0]; sum2 += b[0];}
    if(n >= 2) {  sum1 += a[1]; sum2 += b[1];}
    if(n == 3) {  sum1 += a[2]; sum2 += b[2];}
  }
  else
  {
  	for (int i = 0; i < n; i++) 
  		sum1 += a[i];
  	for (int i = 0; i < n; i++) {
  		sum2 += b[i];
  	}
  }
	return sum1 + sum2;
}

int main(){

	return 0;
}

