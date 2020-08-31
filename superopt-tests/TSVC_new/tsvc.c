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
TYPE temp;

__attribute__((aligned(16))) TYPE a[LEN],b[LEN],c[LEN],d[LEN],e[LEN],
                                   aa[LEN2][LEN2],bb[LEN2][LEN2],cc[LEN2][LEN2],tt[LEN2][LEN2];


TYPE indx[LEN] __attribute__((aligned(16)));


TYPE* /*__restrict__*/ xx;
TYPE* yy;
TYPE arr[LEN];


int s111()
{

//	linear dependence testing
//	no dependence - vectorizable

		for (int i = 1; i < LEN; i += 2) {
			a[i] = a[i - 1] + b[i];
		}
	return 0;
}

int s1111()
{

//	no dependence - vectorizable
//	jump in data access

		for (int i = 0; i < LEN/2; i++) {
			a[2*i] = c[i] * b[i] + d[i] * b[i] + c[i] * c[i] + d[i] * b[i] + d[i] * c[i];
		}
	return 0;
}

int s113()
{

//	linear dependence testing
//	a(i)=a(1) but no actual dependence cycle

		for (int i = 1; i < LEN; i++) {
			a[i] = a[0] + b[i];
		}
	return 0;
}

int s1115()
{

//	linear dependence testing
//	triangular saxpy loop
		for (int i = 0; i < LEN2; i++) {
			for (int j = 0; j < LEN2; j++) {
				aa[i][j] = aa[i][j]*cc[j][i] + bb[i][j];
			}
		}
	return 0;
}

int s1119()
{

//	linear dependence testing
//	no dependence - vectorizable
		for (int i = 1; i < LEN2; i++) {
			for (int j = 0; j < LEN2; j++) {
				aa[i][j] = aa[i-1][j] + bb[i][j];
			}
		}
	return 0;
}

int s114()
{

//	linear dependence testing
//	transpose vectorization
//	Jump in data access - not vectorizable

		for (int i = 0; i < LEN2; i++) {
			for (int j = 0; j < i; j++) {
				aa[i][j] = aa[j][i] + bb[i][j];
			}
		}
	return 0;
}

int s116()
{

//	linear dependence testing
	int i1 = 0;  
	for (int i = 0; i < LEN-5 ; i += 5, i1++) {
		a[i] = a[i + 1] * a[i];
		a[i + 1] = a[i + 2] * a[i + 1];
		a[i + 2] = a[i + 3] * a[i + 2];
		a[i + 3] = a[i + 4] * a[i + 3];
		a[i + 4] = a[i + 5] * a[i + 4];
	}
	return 0;
}

int s119()
{

//	linear dependence testing
//	no dependence - vectorizable

		for (int i = 1; i < LEN2; i++) {
			for (int j = 1; j < LEN2; j++) {
				aa[i][j] = aa[i-1][j-1] + bb[i][j];
			}
		}
	return 0;
}

int s1161()
{

//	control flow
//	tests for recognition of loop independent dependences
//	between statements in mutually exclusive regions.

		for (int i = 0; i < LEN-1; ++i) {
			if (c[i] < (TYPE)0.) {
				goto L20;
			}
			a[i] = c[i] + d[i] * e[i];
			goto L10;
L20:
			b[i] = a[i] + d[i] * d[i];
L10:
			;
		}
	return 0;
}

int s1213()
{

//	statement reordering
//	dependency needing temporary

		for (int i = 1; i < LEN-1; i++) {
			a[i] = b[i-1]+c[i];
			b[i] = a[i+1]*d[i];
		}
	return 0;
}

int s124()
 {
 
 //	induction variable recognition
 //	induction variable under both sides of if (same value)
 
 	int j;
 		j = -1;
 		for (int i = 0; i < LEN; i++) {
 			if (b[i] > (TYPE)0.) {
 				j++;
 				a[j] = b[i] + d[i] * e[i];
 			} else {
 				j++;
 				a[j] = c[i] + d[i] * e[i];
 			}
 		}
 	return 0;
 }

int s125()
{

//	induction variable recognition
//	induction variable in two loops; collapsing possible

	int k;
		k = -1;
		for (int i = 0; i < LEN2; i++) {
			for (int j = 0; j < LEN2; j++) {
				k++;
				array[k] = aa[i][j] + bb[i][j] * cc[i][j];
			}
		}
	return 0;
}

int s1279()
{

//	control flow
//	vector if/gotos

		for (int i = 0; i < LEN; i++) {
			if (a[i] < (TYPE)0.) {
				if (b[i] > a[i]) {
					c[i] += d[i] * e[i];
				}
			}
		}
	return 0;
}

int s128()
{

//	induction variables
//	coupled induction variables
//	jump in data access

	int j, k;
		j = -1;
		for (int i = 0; i < LEN/2; i++) {
			k = j + 1;
			a[i] = b[k] - d[i];
			j = k + 1;
			b[k] = a[i] + c[k];
		}
	return 0;
}

int s131()
{
//	global data flow analysis
//	forward substitution

	int m  = 2;
		for (int i = 0; i < LEN - 2; i++) {
			a[i] = a[i + m] + b[i];
		}
	return 0;
}

int s132()
{
//	global data flow analysis
//	loop with multiple dimension ambiguous subscripts


	int m = 0;
	int j = m;
	int k = m+1;
		for (int i= 1; i < LEN2; i++) {
			aa[j][i] = aa[k][i-1] + b[i] * c[1];
		}
	return 0;
}

int s1421()
{

//	storage classes and equivalencing
//	equivalence- no overlap

	xx = &b[LEN/2];
		for (int i = 0; i < LEN/2; i++) {
			b[i] = xx[i] + a[i];
		}
	int sum = 0;
	for (int i = 0; i < LEN/2; i++){
		sum += xx[i]; // store sinking in gcc,clang
	}
  temp =sum;
	return 0;
}

int s171(int inc)
{

//	symbolics
//	symbolic dependence tests

		for (int i = 0; i < LEN; i++) {
			a[i * inc] += b[i];
		}
	return 0;
}

int s174(int M)
{

//	symbolics
//	loop with subscript that may seem ambiguous

		for (int i = 0; i < M; i++) {
			a[i+M] = a[i] + b[i];
		}
	return 0;
}

int s2233()
{

//	loop interchange
//	interchanging with one of two inner loops

		for (int i = 1; i < LEN2; i++) {
			for (int j = 1; j < LEN2; j++) {
				aa[j][i] = aa[j-1][i] + cc[j][i];
			}
			for (int j = 1; j < LEN2; j++) {
				bb[i][j] = bb[i-1][j] + cc[i][j];
			}
		}
	return 0;
}

int s252()
{

//	scalar and array expansion
//	loop with ambiguous scalar temporary

	TYPE t, s;
		t = (TYPE) 0.;
		for (int i = 0; i < LEN; i++) {
			s = b[i] * c[i];
			a[i] = s + t;
			t = s;
		}
	return 0;
}

int s253()
{

//	scalar and array expansion
//	scalar expansio assigned under if

	TYPE s;
		for (int i = 0; i < LEN; i++) {
			if (a[i] > b[i]) {
				s = a[i] - b[i] * d[i];
				c[i] += s;
				a[i] = s;
			}
		}
	return 0;
}

int s254()
{

  //	scalar and array expansion
  //	carry around variable

	TYPE x;
	x = b[LEN-1];
	for (int i = 0; i < LEN; i++) {
		a[i] = (b[i] + x) * (TYPE).5;
//		a[i] = (b[i] + x) / 2;
		x = b[i];
	}
	return 0;
}

int s271()
{

//	control flow
//	loop with singularity handling
		for (int i = 0; i < LEN; i++) {
			if (b[i] > (TYPE)0.) {
				a[i] += b[i] * c[i];
			}
		}
	return 0;
}

int s2710( TYPE x)
{

//	control flow
//	scalar and vector ifs

		for (int i = 0; i < LEN; i++) {
			if (a[i] > b[i]) {
				a[i] += b[i] * d[i];
				if (LEN > 10) {
					c[i] += d[i] * d[i];
				} else {
					c[i] = d[i] * e[i] + (TYPE)1.;
				}
			} else {
				b[i] = a[i] + e[i] * e[i];
				if (x > (TYPE)0.) {
					c[i] = a[i] + d[i] * d[i];
				} else {
					c[i] += e[i] * e[i];
				}
			}
		}
	return 0;
}

int s2711()
{

//	control flow
//	semantic if removal

		for (int i = 0; i < LEN; i++) {
			if (b[i] != (TYPE)0.0) {
				a[i] += b[i] * c[i];
			}
		}
	return 0;
}

int s2712()
{

//	control flow
//	if to elemental min

		for (int i = 0; i < LEN; i++) {
			if (a[i] > b[i]) {
				a[i] += b[i] * c[i];
			}
		}
	return 0;
}

int s272(TYPE t)
{

//	control flow
//	loop with independent conditional

		for (int i = 0; i < LEN; i++) {
			if (e[i] >= t) {
				a[i] += c[i] * d[i];
				b[i] += c[i] * c[i];
			}
		}
	return 0;
}

int s273()
{

//	control flow
//	simple loop with dependent conditional

		for (int i = 0; i < LEN; i++) {
			a[i] += d[i] * e[i];
			if (a[i] < (TYPE)0.)
				b[i] += d[i] * e[i];
			c[i] += a[i] * d[i];
		}
	return 0;
}


int s274()
{

//	control flow
//	complex loop with dependent conditional

		for (int i = 0; i < LEN; i++) {
			a[i] = c[i] + e[i] * d[i];
			if (a[i] > (TYPE)0.) {
				b[i] = a[i] + b[i];
			} else {
				a[i] = d[i] * e[i];
			}
		}
	return 0;
}

int s276()
{

//	control flow
//	if test using loop index

	int mid = (LEN/2);
		for (int i = 0; i < LEN; i++) {
			if (i+1 < mid) {
				a[i] += b[i] * c[i];
			} else {
				a[i] += b[i] * d[i];
			}
		}
	return 0;
}

int s293()
{

//	loop peeling
//	a(i)=a(0) with actual dependence cycle, loop is vectorizable

		for (int i = 0; i < LEN; i++) {
			a[i] = a[0];
		}
	return 0;
}
int s311()
{

//	reductions
//	sum reduction

	TYPE sum;
		sum = (TYPE)0.;
		for (int i = 0; i < LEN; i++) {
			sum += a[i];
		}
		temp = sum;
	return 0;
}

int s3111()
{

//	reductions
//	conditional sum reduction

	TYPE sum;
		sum = 0.;
		for (int i = 0; i < LEN; i++) {
			if (a[i] > (TYPE)0.) {
				sum += a[i];
			}
		}
	temp = sum;
	return 0;
}

int s317()
{

//	reductions
//	product reductio vectorize with
//	1. scalar expansion of factor, and product reduction
//	2. closed form solution: q = factor**n

	TYPE q;
		q = (TYPE)1.;
		for (int i = 0; i < LEN/2; i++) {
			q *= (TYPE)99;
		}
	temp = q;
	return 0;
}

int s319()
{

//	reductions
//	coupled reductions

	TYPE sum;
		sum = 0.;
		for (int i = 0; i < LEN; i++) {
			a[i] = c[i] + d[i];
			sum += a[i];
			b[i] = c[i] + e[i];
			sum += b[i];
		}
	temp = sum;
	return 0;
}

#ifndef icc_compiler
int s331()
{

//	search loops
//	if to last-1

	int j;
	TYPE chksum;
		j = -1;
		for (int i = 0; i < LEN; i++) {
			if (a[i] < (TYPE)0.) {
				j = i;
			}
		}
		chksum = (TYPE) j;
	temp = j+1;
	return 0;
}
#endif

int s352()
{

//	loop rerolling
//	unrolled dot product

	TYPE dot;
		dot = (TYPE)0.;
	  int i1=0;
		for (int i = 0; i < LEN; i += 5) {
			i1++;
			dot = dot + a[i] * b[i] + a[i + 1] * b[i + 1] + a[i + 2]
				* b[i + 2] + a[i + 3] * b[i + 3] + a[i + 4] * b[i + 4];
		}
	temp = dot;
	return 0;
}

int s4115(int* /*__restrict__*/ ip)
{

//	indirect addressing
//	sparse dot product
//	gather is required

	TYPE sum;
		sum = 0.;
		for (int i = 0; i < LEN; i++) {
			sum += a[i] * b[ip[i]];
		}
	temp = sum;
	return 0;
}

int s421()
{

//	storage classes and equivalencing
//	equivalence- no overlap

		yy = arr;
		for (int i = 0; i < LEN - 1; i++) {
			arr[i] = yy[i+1] + a[i];
		}
	int sum = 0;
	for (int i = 0; i < LEN; i++){
		sum += arr[i];
	}
  temp =sum;
	return 0;
}

int s423()
{

//	storage classes and equivalencing
//	common and equivalenced variables - with anti-dependence

	int vl = 64;
	xx = array+vl;
		for (int i = 0; i < LEN - 1; i++) {
			array[i+1] = xx[i] + a[i];
		}
  int sum = 0;
	for (int i = 0; i < LEN; i++){
		sum += array[i]; // store sinking in gcc, clang
	}
  temp =sum;
	return 0;
}

int s441()
 {
 
 //	non-logical if's
 //	arithmetic if
 
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

int s442()
{

//	non-logical if's
//	computed goto

		for (int i = 0; i < LEN; i++) {
			switch (indx[i]) {
				case 1:  goto L15;
				case 2:  goto L20;
				case 3:  goto L30;
				case 4:  goto L40;
			}
L15:
			a[i] += b[i] * b[i];
			goto L50;
L20:
			a[i] += c[i] * c[i];
			goto L50;
L30:
			a[i] += d[i] * d[i];
			goto L50;
L40:
			a[i] += e[i] * e[i];
L50:
			;
		}
	return 0;
}

 int s443()
 {
 
 //	non-logical if's
 //	arithmetic if
 
 		for (int i = 0; i < LEN; i++) {
 			if (d[i] <= (TYPE)0.) {
 				goto L20;
 			} else {
 				goto L30;
 			}
 L20:
 			a[i] += b[i] * c[i];
 			goto L50;
 L30:
 			a[i] += b[i] * b[i];
 L50:
 			;
 		}
 	return 0;
 }

int s471(){

	int m = LEN;
		for (int i = 0; i < m; i++) {
			x[i] = b[i] + d[i] * d[i];
			b[i] = c[i] + d[i] * e[i];
		}
  int sum = 0;
	for (int i = 0; i < LEN; i++){
		sum += x[i]; // store sinking in gcc,clang
	}
  temp =sum;
	return 0;
}

int va()
{

//	control loops
//	vector assignment

		for (int i = 0; i < LEN; i++) {
			a[i] = b[i];
		}
	return 0;
}

int vbor()
{

//	control loops
//	basic operations rates, isolate arithmetic from memory traffic
//	all combinations of three, 59 flops for 6 loads and 1 store.

	TYPE a1, b1, c1, d1, e1, f1;
		for (int i = 0; i < LEN2; i++) {
			a1 = a[i];
			b1 = b[i];
			c1 = c[i];
			d1 = d[i];
			e1 = e[i];
			f1 = aa[0][i];
			a1 = a1 * b1 * c1 + a1 * b1 * d1 + a1 * b1 * e1 + a1 * b1 * f1 +
				a1 * c1 * d1 + a1 * c1 * e1 + a1 * c1 * f1 + a1 * d1 * e1
				+ a1 * d1 * f1 + a1 * e1 * f1;
			b1 = b1 * c1 * d1 + b1 * c1 * e1 + b1 * c1 * f1 + b1 * d1 * e1 +
				b1 * d1 * f1 + b1 * e1 * f1;
			c1 = c1 * d1 * e1 + c1 * d1 * f1 + c1 * e1 * f1;
			d1 = d1 * e1 * f1;
			x[i] = a1 * b1 * c1 * d1;
		}
  int sum = 0;
	for (int i = 0; i < LEN; i++){
		sum += x[i];
	}
	temp = sum;
	return 0;
}

int vif()
{

//	control loops
//	vector if

		for (int i = 0; i < LEN; i++) {
			if (b[i] > (TYPE)0.) {
				a[i] = b[i];
			}
		}
	return 0;
}

int main()
{
  return 0;
}

