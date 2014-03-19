#include "test.h"
#define NQ 64
#define NR 64
#define NP 64

void doitgen(int A[__restrict__ NR][NQ][NP], int sum[__restrict__ NR][NQ][NP], int C4[__restrict__ NP][NP])
{
	int r, q, p, s ; 

	#pragma omp parallel for
	for (r = 0 ; r < NR ; r++) {
		for (q = 0 ; q < NQ ; q++)
			for (p = 0; p < NP; p++)
				A[r][q][p] = ((int) r*q + p) / NP ;
	}

	#pragma omp parallel for
	for (r = 0; r < NP; r++) {
		for (q = 0; q < NP; q++)
			C4[r][q] = ((int) r*q) / NP ;
	}

	#pragma omp parallel for
	for (r = 0; r < NR; r++) {
		for (q = 0; q < NQ; q++) {
			for (p = 0; p < NP; p++) {
				sum[r][q][p] = 0;
				for (s = 0; s < NP; s++)
					sum[r][q][p] = sum[r][q][p] + A[r][q][s] * C4[s][p];
			}   
			for (p = 0; p < NR; p++)
				A[r][q][p] = sum[r][q][p];
		}   
	}
}

int main()
{
	int A[NR][NQ][NP] ;
	int sum[NR][NQ][NP] ;
	int C4[NP][NP] ;

	int i ;
	for(i=0 ; i<NUM_ITER ; i++) {
		doitgen(A, sum, C4) ;
	}

	return 0 ;
}

void cleanup()
{
}
