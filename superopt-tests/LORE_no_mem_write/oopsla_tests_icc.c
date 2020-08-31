/* LORE LOOP NESTS -- Functions having atleat one loop nest without Mem write */
//Functions auto vectorized by ICC

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

//3-way branch 1D loop
//Sum
int ex7()
{

  int sum = 0;
	for (int i = 0; i < LEN; i++) {
		if (d[i] < (TYPE)0.) {
			sum--;
		} else if (d[i] == (TYPE)0.) {
			sum = sum;
		} else {
			sum++;
		}
	}
	return sum;
}

int main(){

	return 0;
}


