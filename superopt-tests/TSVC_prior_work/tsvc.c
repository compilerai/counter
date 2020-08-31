
/* TSVC functions vectorized by either of GCC, CLANG or ICC and not demonstrated by any prior work */


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

#define lll LEN


TYPE val = 1;
__attribute__ ((aligned(16))) TYPE X[lll],Y[lll],Z[lll],U[lll],V[lll];

TYPE array[LEN2*LEN2] __attribute__((aligned(16)));

TYPE x[LEN] __attribute__((aligned(16)));
TYPE temp, s;

__attribute__((aligned(16))) TYPE a[LEN],b[LEN],c[LEN],d[LEN],e[LEN],
                                   aa[LEN2][LEN2],bb[LEN2][LEN2],cc[LEN2][LEN2],tt[LEN2][LEN2];


TYPE indx[LEN] __attribute__((aligned(16)));


TYPE* /*__restrict__*/ xx;
TYPE* yy;
TYPE arr[LEN];

int s000()
{

//	linear dependence testing
//	no dependence - vectorizable

		for (int i = 0; i < lll; i++) {
			X[i] = Y[i] + val;
		}
	return 0;
}

int s1112()
{

//	linear dependence testing
//	loop reversal

		for (int i = LEN - 1; i >= 0; i--) {
			a[i] = b[i] + val;
		}
	return 0;
}

int s112()
{

//	linear dependence testing
//	loop reversal

		for (int i = LEN - 2; i >= 0; i--) {
			a[i+1] = a[i] + b[i];
		}
	return 0;
}
int s121()
{

//	induction variable recognition
//	loop with possible ambiguity because of scalar store
    int j;
		for (int i = 0; i < LEN-1; i++) {
			j = i + 1;
			a[i] = a[j] + b[i];
		}
	return 0;
}
int s122(int n1, int n3)
{

//	induction variable recognition
//	variable lower and upper bound, and stride
//	reverse data access and jump in data access

	int j, k;
		j = 1;
		k = 0;
		for (int i = n1-1; i < LEN; i += n3) {
			k += j;
			a[i] += b[LEN - k];
		}
	return 0;
}
int s1221()
{

//	run-time symbolic resolution

		for (int i = 4; i < LEN; i++) {
			b[i] = b[i - 4] + a[i];
		}
	return 0;
}
int s1251()
{

//	scalar and array expansion
//	scalar expansion

	TYPE s;
		for (int i = 0; i < LEN; i++) {
			s = b[i]+c[i];
			b[i] = a[i]+d[i];
			a[i] = s*e[i];
		}
	return 0;
}
int s127()
{

//	induction variable recognition
//	induction variable with multiple increments

	int j;
		j = -1;
		for (int i = 0; i < LEN/2; i++) {
			j++;
			a[j] = b[i] + c[i] * d[i];
			j++;
			a[j] = b[i] + d[i] * e[i];
		}
	return 0;
}
int s1281()
{

//	crossing thresholds
//	index set splitting
//	reverse data access

	TYPE x;
		for (int i = 0; i < LEN; i++) {
			x = b[i]*c[i]+a[i]*d[i]+e[i];
			a[i] = x-(TYPE)1.0;
			b[i] = x;
		}
	return 0;
}
int s1351()
{

//	induction pointer recognition

		TYPE* __restrict__ A = a;
		TYPE* __restrict__ B = b;
		TYPE* __restrict__ C = c;
		for (int i = 0; i < LEN; i++) {
			*A = *B+*C;
			A++;
			B++;
			C++;
		}
	return 0;
}
int s162(int k)
{
//	control flow
//	deriving assertions

		if (k > 0) {
			for (int i = 0; i < LEN-1; i++) {
				a[i] = a[i + k] + b[i] * c[i];
			}
		}
	return 0;
}
int s173()
{
//	symbolics
//	expression in loop bounds and subscripts

	int k = LEN/2;
		for (int i = 0; i < LEN/2; i++) {
			a[i+k] = a[i] + b[i];
		}
	return 0;
}
int s176()
{

//	symbolics
//	convolution
	int m = LEN/2;
		for (int j = 0; j < (LEN/2); j++) {
			for (int i = 0; i < m; i++) {
				a[i] += b[i+m-j-1] * c[j];
			}
//			DBG(__LINE__);
		}
	return 0;
}

int s2244()
{

//	node splitting
//	cycle with ture and anti dependency

		for (int i = 0; i < LEN-1; i++) {
			a[i+1] = b[i] + e[i];
			a[i] = b[i] + c[i];
		}
	return 0;
}
int s251()
{

//	scalar and array expansion
//	scalar expansion

	TYPE s;
		for (int i = 0; i < LEN; i++) {
			s = b[i] + c[i] * d[i];
			a[i] = s * s;
		}
	return 0;
}

int s243()
{

//	node splitting
//	false dependence cycle breaking

		for (int i = 0; i < LEN-1; i++) {
			a[i] = b[i] + c[i  ] * d[i];
			b[i] = a[i] + d[i  ] * e[i];
			a[i] = b[i] + a[i+1] * d[i];
		}
	return 0;
}

int s3251()
{

//	scalar and array expansion
//	scalar expansion

		for (int i = 0; i < LEN-1; i++){
			a[i+1] = b[i]+c[i];
			b[i]   = c[i]*e[i];
			d[i]   = a[i]*e[i];
		}
	return 0;
}

int s351()
{

//	loop rerolling
//	unrolled saxpy

	TYPE alpha = c[0];
	int i1 = 0;
		for (int i = 0; i < LEN; i = i+5) {
		  i1++;
			a[i] += alpha * b[i];
			a[i + 1] += alpha * b[i + 1];
			a[i + 2] += alpha * b[i + 2];
			a[i + 3] += alpha * b[i + 3];
			a[i + 4] += alpha * b[i + 4];
		}
	return 0;
}



int s452()
{

//	intrinsic functions
//	seq function
		for (int i = 0; i < LEN; i++) {
			a[i] = b[i] + c[i] * (TYPE) (val);
			a[i] = a[i] + c[i];
		}
	return 0;
}


int val_s;
int s453()
{

//	induction varibale recognition

		for (int i = 0; i < LEN; i++) {
			a[i] = val_s * b[i];
		}
	return 0;
}
int vtv()
{

//	control loops
//	vector times vector

		for (int i = 0; i < LEN; i++) {
			a[i] *= b[i];
		}
	return 0;
}


int vpvtv()
{

//	control loops
//	vector plus vector times vector

		for (int i = 0; i < LEN; i++) {
			a[i] += b[i] * c[i];
		}
	return 0;
}


int vpvts()
{

//	control loops
//	vector plus vector times scalar

		for (int i = 0; i < LEN; i++) {
			a[i] += b[i] * s;
		}
	return 0;
}


int vpvpv()
{

//	control loops
//	vector plus vector plus vector

		for (int i = 0; i < LEN; i++) {
			a[i] += b[i] + c[i];
		}
	return 0;
}

int vtvtv()
{

//	control loops
//	vector times vector times vector

		for (int i = 0; i < LEN; i++) {
			a[i] = a[i] * b[i] * c[i];
		}
	return 0;
}

int vpv()
{

//	control loops
//	vector plus vector

		for (int i = 0; i < LEN; i++) {
			a[i] += b[i];
		}
	return 0;
}

int vdotr()
{

//	control loops
//	vector dot product reduction

	TYPE dot;
		dot = 0.;
		for (int i = 0; i < LEN; i++) {
			dot += a[i] * b[i];
		}
	temp = dot;
	return 0;
}

TYPE sum1d(){
	TYPE ret = 0.;
	for (int i = 0; i < LEN; i++)
		ret += arr[i];
	return ret;
}

int main()
{
  return 0;
}
