/* LORE LOOP NESTS -- Functions with all loop nests having atleast one Mem write */
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

//MW-3d
TYPE ex106(){
	for (int i = 0; i < LEN3; i++)
	  for (int j = 0; j < LEN3; j++)
      #pragma GCC unroll 2
	    for (int k = 0; k < LEN3; k++)
		    bbb[i][j][k] = aaa[i][j][k];
	return 0;
}

//3-way branch 1D loop
//MW
int ex107()
{

    #pragma GCC unroll 2
    for (int i = 0; i < LEN; i++) {
      if (d[i] < (TYPE)0.) {
        a[i] += b[i] * c[i];
      } else if (d[i] == (TYPE)0.) {
        a[i] += b[i] * b[i];
      } else {
        a[i] += c[i] * c[i];
      }   
    }   
  return 0;
}


int main(){

	return 0;
}

