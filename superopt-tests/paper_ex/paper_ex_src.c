/* The example function shown in Fig 1 of the paper */


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

int a[100][50];
int nestedLoop(){
  int sum = 0;
  for ( int i =0; i <100; i++) {
    for ( int j = i ; j <50; j++) {
      sum += a[i][j];
    }
  }
  return sum;
}

int main()
{
  return 0;
}

