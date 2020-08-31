/* LORE LOOP NESTS -- Functions having atleast one loop nest without Mem write */
// Functions auto vectorized by GCC for unroll factor 4; Using pragma to generate unroll factor 8 code

#define LEN 32000
#define LEN1 3200
#define LEN2 1024
#define LEN3 256

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
                                   aa[LEN2][LEN2],bb[LEN2][LEN2],cc[LEN2][LEN2],dd[LEN2][LEN2],tmp[LEN2][LEN2], aaa[LEN3][LEN3][LEN3], bbb[LEN3][LEN3][LEN3];

//sum2d
TYPE ex2(){
	TYPE ret = 0.;
	for (int i = 0; i < LEN2; i++)
    #pragma GCC unroll 2
	  for (int j = 0; j < LEN2; j++)
		  ret += aa[i][j];
	return ret;
}

//sum2d-1d
TYPE ex3(){
	TYPE ret = 0.;
	for (int i = 0; i < LEN2; i++)
    #pragma GCC unroll 2
	  for (int j = 0; j < LEN2; j++)
		 ret += aa[i][j];

  #pragma GCC unroll 2
	for (int i = 0; i < LEN; i++)
		ret += a[i];
	return ret;
}

//sum2d-1d-imperfect
TYPE ex4(){
	TYPE ret = 0.;
	for (int i = 0; i < LEN2; i++) {
     #pragma GCC unroll 2
	   for (int j = 0; j < LEN2; j++)
		  ret += aa[i][j];
		ret += a[i];
  }
	return ret;
}

//sum1d-2d
TYPE ex5(){
	TYPE ret = 0.;
  #pragma GCC unroll 2
	for (int i = 0; i < LEN; i++)
		ret += a[i];
	for (int i = 0; i < LEN2; i++)
    #pragma GCC unroll 2
	  for (int j = 0; j < LEN2; j++)
		  ret += aa[i][j];
	return ret;
}

//sum3d
TYPE ex6(){
	TYPE ret = 0.;
	for (int i = 0; i < LEN3; i++)
	  for (int j = 0; j < LEN3; j++)
      #pragma GCC unroll 2
	    for (int k = 0; k < LEN3; k++)
		    ret += aaa[i][j][k];
	return ret;
}

//loop 3D (2-sum MW) imperfect 
void ex13()
//void example14() 
{
  int k,j,i=0;
  int len = LEN2/2;
  for (k = 0; k < len; k++) {
    int sum = 0;
    for (i = 0; i < len; i++)
      #pragma GCC unroll 2
      for (j = 0; j < len; j++)
          sum += aa[i+k][j] * bb[i][j];

    a[k] = sum;
  }

}

int main(){

	return 0;
}

